#include "texture.hpp"

#include "core/log_assert.hpp"
#include "render/graphics_result.hpp"
#include "render/vulkan/vulkan_buffer.hpp"

namespace ez
{
TextureCreationInfo::~TextureCreationInfo()
{
    // todo: mewmew fix leak
    // if (buffer != nullptr) { delete[] buffer; }
}

TextureCreationInfo TextureCreationInfo::CreateFromData(unsigned char* data,
                                                        uint32_t width,
                                                        uint32_t height,
                                                        uint32_t channelsCount,
                                                        const TextureSampler& textureSampler)
{
    TextureCreationInfo ci;

    ci.bufferSize = static_cast<vk::DeviceSize>(width * height * 4);
    ci.buffer = new unsigned char[ci.bufferSize];

    unsigned char* copyTo = ci.buffer;
    unsigned char* copyFrom = data;
    for (uint32_t i = 0; i < width * height; ++i)
    {
        for (uint32_t j = 0; j < channelsCount; ++j) { copyTo[j] = copyFrom[j]; }
        copyTo += 4;
        copyFrom += channelsCount;
    }

    ci.width = width;
    ci.height = height;
    ci.mipLevels =
        static_cast<uint32_t>(std::floor(std::log2(std::max(ci.width, ci.height))) + 1.0);

    ci.textureSampler = textureSampler;

    return ci;
}

bool TextureCreationInfo::IsValid() const
{
    return width > 0 && height > 0 && bufferSize > 0 && buffer != nullptr;
}

bool Texture::LoadToGpu(vk::Device aLogicalDevice,
                        vk::PhysicalDevice physicalDevice,
                        vk::Queue graphicsQueue,
                        vk::CommandPool graphicsCommandPool)
{
    if (loadedToGpu)
    {
        EZASSERT(false, "Trying to load texture to GPU but it is already loaded");
        return true;
    }
    if (!creationInfo.IsValid())
    {
        EZASSERT(false, "Can't load Texture with incomplete TextureCreationInfo");
        return false;
    }
    // notice CPU texture data in creationInfo is not freed after load

    width = creationInfo.width;
    height = creationInfo.height;
    mipLevels = creationInfo.mipLevels;

    logicalDevice = aLogicalDevice;

    vk::Format format = vk::Format::eR8G8B8A8Unorm;
    vk::FormatProperties formatProperties;

    physicalDevice.getFormatProperties(format, &formatProperties);

    EZASSERT(static_cast<bool>(formatProperties.optimalTilingFeatures &
                               vk::FormatFeatureFlagBits::eBlitSrc));
    EZASSERT(static_cast<bool>(formatProperties.optimalTilingFeatures &
                               vk::FormatFeatureFlagBits::eBlitDst));

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingMemory;

    vk::BufferCreateInfo bufferCI{};
    bufferCI.setSize(creationInfo.bufferSize);
    bufferCI.setUsage(vk::BufferUsageFlags(vk::BufferUsageFlagBits::eTransferSrc));
    bufferCI.setSharingMode(vk::SharingMode::eExclusive);

    CheckVkResult(logicalDevice.createBuffer(&bufferCI, nullptr, &stagingBuffer));

    vk::MemoryRequirements memReqs{};
    vk::MemoryAllocateInfo memAllocInfo{};
    logicalDevice.getBufferMemoryRequirements(stagingBuffer, &memReqs);

    const uint32_t memoryTypeIndex = VulkanBuffer::FindMemoryType(
        physicalDevice,
        memReqs.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    memAllocInfo.setAllocationSize(memReqs.size);
    memAllocInfo.setMemoryTypeIndex(memoryTypeIndex);

    CheckVkResult(logicalDevice.allocateMemory(&memAllocInfo, nullptr, &stagingMemory));
    CheckVkResult(logicalDevice.bindBufferMemory(stagingBuffer, stagingMemory, 0));

    uint8_t* data;
    CheckVkResult(logicalDevice.mapMemory(
        stagingMemory, 0, memReqs.size, vk::MemoryMapFlags{}, reinterpret_cast<void**>(&data)));
    memcpy(data, creationInfo.buffer, creationInfo.bufferSize);
    logicalDevice.unmapMemory(stagingMemory);

    // /////////////////////////////

    vk::ImageCreateInfo imageCreateInfo{};
    imageCreateInfo.setImageType(vk::ImageType::e2D);
    imageCreateInfo.setFormat(format);
    imageCreateInfo.setMipLevels(creationInfo.mipLevels);
    imageCreateInfo.setArrayLayers(1);
    imageCreateInfo.setSamples(vk::SampleCountFlagBits::e1);
    imageCreateInfo.setTiling(vk::ImageTiling::eOptimal);
    imageCreateInfo.setUsage(vk::ImageUsageFlagBits::eSampled);
    imageCreateInfo.setSharingMode(vk::SharingMode::eExclusive);
    imageCreateInfo.setInitialLayout(vk::ImageLayout::eUndefined);
    imageCreateInfo.setExtent(vk::Extent3D(width, height, 1));
    imageCreateInfo.setUsage(vk::ImageUsageFlagBits::eTransferDst |
                             vk::ImageUsageFlagBits::eTransferSrc |
                             vk::ImageUsageFlagBits::eSampled);

    CheckVkResult(logicalDevice.createImage(&imageCreateInfo, nullptr, &image));

    logicalDevice.getImageMemoryRequirements(image, &memReqs);
    const uint32_t imageLocalMemoryTypeIndex = VulkanBuffer::FindMemoryType(
        physicalDevice, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = imageLocalMemoryTypeIndex;
    CheckVkResult(logicalDevice.allocateMemory(&memAllocInfo, nullptr, &deviceMemory));
    CheckVkResult(logicalDevice.bindImageMemory(image, deviceMemory, 0));

    vk::CommandBufferAllocateInfo allocInfo = {};  // todo: one-time CB create-submit helper
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = graphicsCommandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer copyCmd;
    CheckVkResult(logicalDevice.allocateCommandBuffers(&allocInfo, &copyCmd));

    vk::CommandBufferBeginInfo beginInfo = {};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    CheckVkResult(copyCmd.begin(&beginInfo));

    vk::ImageSubresourceRange subresourceRange = {};
    subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
    subresourceRange.setLevelCount(1);
    subresourceRange.setLayerCount(1);

    {
        vk::ImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.setOldLayout(vk::ImageLayout::eUndefined);
        imageMemoryBarrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
        imageMemoryBarrier.setSrcAccessMask(vk::AccessFlags{});
        imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
        imageMemoryBarrier.setImage(image);
        imageMemoryBarrier.setSubresourceRange(subresourceRange);
        copyCmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                                vk::PipelineStageFlagBits::eAllCommands,
                                vk::DependencyFlags{},
                                0,
                                nullptr,
                                0,
                                nullptr,
                                1,
                                &imageMemoryBarrier);
    }

    vk::BufferImageCopy bufferCopyRegion = {};
    bufferCopyRegion.imageSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
    bufferCopyRegion.imageSubresource.setMipLevel(0);
    bufferCopyRegion.imageSubresource.setBaseArrayLayer(0);
    bufferCopyRegion.imageSubresource.setLayerCount(1);
    bufferCopyRegion.imageExtent.setWidth(width);
    bufferCopyRegion.imageExtent.setHeight(height);
    bufferCopyRegion.imageExtent.setDepth(1);

    copyCmd.copyBufferToImage(
        stagingBuffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &bufferCopyRegion);

    {
        vk::ImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
        imageMemoryBarrier.setNewLayout(vk::ImageLayout::eTransferSrcOptimal);
        imageMemoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);
        imageMemoryBarrier.setImage(image);
        imageMemoryBarrier.setSubresourceRange(subresourceRange);
        copyCmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                                vk::PipelineStageFlagBits::eAllCommands,
                                vk::DependencyFlags{},
                                0,
                                nullptr,
                                0,
                                nullptr,
                                1,
                                &imageMemoryBarrier);
    }

    CheckVkResult(copyCmd.end());

    vk::SubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCmd;

    CheckVkResult(graphicsQueue.submit(1, &submitInfo, nullptr));
    CheckVkResult(graphicsQueue.waitIdle());

    logicalDevice.freeCommandBuffers(graphicsCommandPool, 1, &copyCmd);
    logicalDevice.destroyBuffer(stagingBuffer);
    logicalDevice.freeMemory(stagingMemory);

    // //////////////////////////////////////////////

    /*
    // Generate the mip chain (glTF uses jpg and png, so we need to create this manually)
    VkCommandBuffer blitCmd =
        device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    for (uint32_t i = 1; i < mipLevels; i++)
    {
        VkImageBlit imageBlit{};

        imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.srcSubresource.layerCount = 1;
        imageBlit.srcSubresource.mipLevel = i - 1;
        imageBlit.srcOffsets[1].x = int32_t(width >> (i - 1));
        imageBlit.srcOffsets[1].y = int32_t(height >> (i - 1));
        imageBlit.srcOffsets[1].z = 1;

        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.dstSubresource.layerCount = 1;
        imageBlit.dstSubresource.mipLevel = i;
        imageBlit.dstOffsets[1].x = int32_t(width >> i);
        imageBlit.dstOffsets[1].y = int32_t(height >> i);
        imageBlit.dstOffsets[1].z = 1;

        VkImageSubresourceRange mipSubRange = {};
        mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        mipSubRange.baseMipLevel = i;
        mipSubRange.levelCount = 1;
        mipSubRange.layerCount = 1;

        {
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.image = image;
            imageMemoryBarrier.subresourceRange = mipSubRange;
            vkCmdPipelineBarrier(blitCmd,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &imageMemoryBarrier);
        }

        vkCmdBlitImage(blitCmd,
                       image,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &imageBlit,
                       VK_FILTER_LINEAR);

        {
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.image = image;
            imageMemoryBarrier.subresourceRange = mipSubRange;
            vkCmdPipelineBarrier(blitCmd,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &imageMemoryBarrier);
        }
    }

    subresourceRange.levelCount = mipLevels;
    imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    {
        VkImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange = subresourceRange;
        vkCmdPipelineBarrier(blitCmd,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &imageMemoryBarrier);
    }

    device->flushCommandBuffer(blitCmd, copyQueue, true);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = textureSampler.magFilter;
    samplerInfo.minFilter = textureSampler.minFilter;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = textureSampler.addressModeU;
    samplerInfo.addressModeV = textureSampler.addressModeV;
    samplerInfo.addressModeW = textureSampler.addressModeW;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.maxAnisotropy = 1.0;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxLod = (float)mipLevels;
    samplerInfo.maxAnisotropy = 8.0f;
    samplerInfo.anisotropyEnable = VK_TRUE;
    VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerInfo, nullptr, &sampler));

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.components = { VK_COMPONENT_SWIZZLE_R,
                            VK_COMPONENT_SWIZZLE_G,
                            VK_COMPONENT_SWIZZLE_B,
                            VK_COMPONENT_SWIZZLE_A };
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.levelCount = mipLevels;
    VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &viewInfo, nullptr, &view));

    descriptor.sampler = sampler;
    descriptor.imageView = view;
    descriptor.imageLayout = imageLayout;
*/
    loadedToGpu = true;
    return true;
}

Texture::~Texture() { Destroy(); }

void Texture::Destroy()
{
    if (logicalDevice)
    {
        logicalDevice.destroyImageView(imageView);
        logicalDevice.destroyImage(image);
        logicalDevice.freeMemory(deviceMemory);
        logicalDevice.destroySampler(sampler);
    }
}

}  // namespace ez
