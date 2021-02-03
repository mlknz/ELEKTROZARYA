#pragma once
#include "render/graphics_result.hpp"
#include "render/vulkan_include.hpp"

namespace ez::Image
{
ResultValue<vk::Image> CreateImage2D(vk::Device logicalDevice,
                                     vk::Format format,
                                     vk::ImageUsageFlags usage,
                                     uint32_t mipLevels,
                                     uint32_t width,
                                     uint32_t height);

ResultValue<vk::ImageView> CreateImageView2D(vk::Device logicalDevice,
                                             vk::Image image,
                                             vk::Format format,
                                             vk::ImageAspectFlagBits aspectMask,
                                             uint32_t mipLevelsCount);

bool GenerateMipsForImage(vk::Device logicalDevice,
                          vk::Queue graphicsQueue,
                          vk::CommandPool graphicsCommandPool,
                          vk::Image image,
                          uint32_t width,
                          uint32_t height,
                          uint32_t mipLevels);

}  // namespace ez::Image
