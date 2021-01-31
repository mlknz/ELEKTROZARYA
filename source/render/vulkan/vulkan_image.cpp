#include "vulkan_image.hpp"

#include "core/log_assert.hpp"

namespace ez::Image
{
ResultValue<vk::Image> CreateImage2D(vk::Device logicalDevice,
                                     vk::Format format,
                                     vk::ImageUsageFlags usage,
                                     uint32_t mipLevels,
                                     uint32_t width,
                                     uint32_t height)
{
    vk::Image image;

    vk::ImageCreateInfo imageCreateInfo{};
    imageCreateInfo.setImageType(vk::ImageType::e2D);
    imageCreateInfo.setFormat(format);
    imageCreateInfo.setMipLevels(mipLevels);
    imageCreateInfo.setArrayLayers(1);
    imageCreateInfo.setSamples(vk::SampleCountFlagBits::e1);
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

}  // namespace ez::Image
