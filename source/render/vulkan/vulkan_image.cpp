#include "vulkan_image.hpp"

#include "core/log_assert.hpp"
#include "render/vulkan/vulkan_buffer.hpp"

namespace ez::Image
{
ResultValue<vk::Image> CreateImage2D(vk::Device logicalDevice,
                                     vk::Format format,
                                     vk::ImageUsageFlags usage,
                                     uint32_t mipLevels,
                                     uint32_t width,
                                     uint32_t height,
                                     vk::SampleCountFlagBits samplesCount)
{
    vk::Image image;

    vk::ImageCreateInfo imageCreateInfo{};
    imageCreateInfo.setImageType(vk::ImageType::e2D);
    imageCreateInfo.setFormat(format);
    imageCreateInfo.setMipLevels(mipLevels);
    imageCreateInfo.setArrayLayers(1);
    imageCreateInfo.setSamples(samplesCount);
    imageCreateInfo.setTiling(vk::ImageTiling::eOptimal);
    imageCreateInfo.setUsage(usage);
    imageCreateInfo.setSharingMode(vk::SharingMode::eExclusive);
    imageCreateInfo.setInitialLayout(vk::ImageLayout::eUndefined);
    imageCreateInfo.setExtent(vk::Extent3D(width, height, 1));

    if (logicalDevice.createImage(&imageCreateInfo, nullptr, &image) != vk::Result::eSuccess)
    {
        EZASSERT(false, "Failed to create image!");
        return GraphicsResult::Error;
    }

    return { GraphicsResult::Ok, image };
}

ResultValue<ImageWithMemory> CreateImage2DWithMemory(vk::Device logicalDevice,
                                                     vk::PhysicalDevice physicalDevice,
                                                     vk::Format format,
                                                     vk::ImageUsageFlags usage,
                                                     uint32_t mipLevels,
                                                     uint32_t width,
                                                     uint32_t height,
                                                     vk::SampleCountFlagBits samplesCount)
{
    ResultValue<vk::Image> imageRV =
        CreateImage2D(logicalDevice, format, usage, mipLevels, width, height, samplesCount);
    if (imageRV.result != GraphicsResult::Ok) { return imageRV.result; }
    vk::Image image = imageRV.value;
    vk::DeviceMemory imageMemory;

    vk::MemoryRequirements memReqs{};
    vk::MemoryAllocateInfo memAllocInfo{};
    logicalDevice.getImageMemoryRequirements(image, &memReqs);
    const uint32_t imageLocalMemoryTypeIndex = VulkanBuffer::FindMemoryType(
        physicalDevice, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = imageLocalMemoryTypeIndex;
    CheckVkResult(logicalDevice.allocateMemory(&memAllocInfo, nullptr, &imageMemory));
    CheckVkResult(logicalDevice.bindImageMemory(image, imageMemory, 0));

    return { GraphicsResult::Ok, { image, imageMemory } };
}

ResultValue<vk::ImageView> CreateImageView2D(vk::Device logicalDevice,
                                             vk::Image image,
                                             vk::Format format,
                                             vk::ImageAspectFlagBits aspectMask,
                                             uint32_t mipLevelsCount)
{
    vk::ImageView imageView;

    vk::ImageViewCreateInfo createInfo = {};
    createInfo.setImage(image)
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(format)
        .setComponents(vk::ComponentMapping{});  // swizzling .rgba
    createInfo.subresourceRange.setAspectMask(aspectMask)
        .setBaseMipLevel(0)
        .setLevelCount(mipLevelsCount)
        .setBaseArrayLayer(0)
        .setLayerCount(1);

    if (logicalDevice.createImageView(&createInfo, nullptr, &imageView) != vk::Result::eSuccess)
    {
        EZASSERT(false, "Failed to create image view!");
        return GraphicsResult::Error;
    }

    return { GraphicsResult::Ok, imageView };
}

bool GenerateMipsForImage(vk::Device logicalDevice,
                          vk::Queue graphicsQueue,
                          vk::CommandPool graphicsCommandPool,
                          vk::Image image,
                          uint32_t width,
                          uint32_t height,
                          uint32_t mipLevels)
{
    vk::ImageSubresourceRange subresourceRange = {};
    subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
    subresourceRange.setLevelCount(1);
    subresourceRange.setLayerCount(1);

    vk::CommandBufferAllocateInfo allocInfo = {};  // todo: one-time CB create-submit helper
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = graphicsCommandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBufferBeginInfo beginInfo = {};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    vk::CommandBuffer blitCmd;
    CheckVkResult(logicalDevice.allocateCommandBuffers(&allocInfo, &blitCmd));
    CheckVkResult(blitCmd.begin(&beginInfo));

    for (uint32_t i = 1; i < mipLevels; i++)
    {
        vk::ImageBlit imageBlit{};

        imageBlit.srcSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
        imageBlit.srcSubresource.setLayerCount(1);
        imageBlit.srcSubresource.setMipLevel(i - 1);
        imageBlit.setSrcOffsets(
            { vk::Offset3D{},
              vk::Offset3D(int32_t(width >> (i - 1)), int32_t(height >> (i - 1)), 1) });

        imageBlit.dstSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
        imageBlit.dstSubresource.setLayerCount(1);
        imageBlit.dstSubresource.setMipLevel(i);
        imageBlit.setDstOffsets(
            { vk::Offset3D{}, vk::Offset3D(int32_t(width >> i), int32_t(height >> i), 1) });

        vk::ImageSubresourceRange mipSubRange = {};
        mipSubRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
        mipSubRange.setBaseMipLevel(i);
        mipSubRange.setLevelCount(1);
        mipSubRange.setLayerCount(1);

        {
            vk::ImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.setOldLayout(vk::ImageLayout::eUndefined);
            imageMemoryBarrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
            imageMemoryBarrier.setSrcAccessMask(vk::AccessFlags{});
            imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
            imageMemoryBarrier.setImage(image);
            imageMemoryBarrier.setSubresourceRange(mipSubRange);
            blitCmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                    vk::PipelineStageFlagBits::eTransfer,
                                    vk::DependencyFlags{},
                                    0,
                                    nullptr,
                                    0,
                                    nullptr,
                                    1,
                                    &imageMemoryBarrier);
        }
        blitCmd.blitImage(image,
                          vk::ImageLayout::eTransferSrcOptimal,
                          image,
                          vk::ImageLayout::eTransferDstOptimal,
                          1,
                          &imageBlit,
                          vk::Filter::eLinear);

        {
            vk::ImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
            imageMemoryBarrier.setNewLayout(vk::ImageLayout::eTransferSrcOptimal);
            imageMemoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
            imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);
            imageMemoryBarrier.setImage(image);
            imageMemoryBarrier.setSubresourceRange(mipSubRange);

            blitCmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                    vk::PipelineStageFlagBits::eTransfer,
                                    vk::DependencyFlags{},
                                    0,
                                    nullptr,
                                    0,
                                    nullptr,
                                    1,
                                    &imageMemoryBarrier);
        }
    }

    subresourceRange.setLevelCount(mipLevels);

    {
        vk::ImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal);
        imageMemoryBarrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        imageMemoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);
        imageMemoryBarrier.setImage(image);
        imageMemoryBarrier.setSubresourceRange(subresourceRange);
        blitCmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                                vk::PipelineStageFlagBits::eAllCommands,
                                vk::DependencyFlags{},
                                0,
                                nullptr,
                                0,
                                nullptr,
                                1,
                                &imageMemoryBarrier);
    }

    CheckVkResult(blitCmd.end());

    vk::SubmitInfo submitInfo = vk::SubmitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &blitCmd;

    CheckVkResult(graphicsQueue.submit(1, &submitInfo, nullptr));
    CheckVkResult(graphicsQueue.waitIdle());

    logicalDevice.freeCommandBuffers(graphicsCommandPool, 1, &blitCmd);

    return true;
}

}  // namespace ez::Image
