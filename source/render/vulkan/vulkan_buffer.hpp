#pragma once

#include "render/vulkan_include.hpp"

namespace ez
{
class VulkanBuffer
{
   public:
    static bool createBuffer(vk::Device logicalDevice,
                             vk::PhysicalDevice physicalDevice,
                             vk::DeviceSize size,
                             vk::BufferUsageFlags usage,
                             vk::MemoryPropertyFlags properties,
                             vk::Buffer& buffer,
                             vk::DeviceMemory& bufferMemory);

    static void copyBuffer(vk::Device logicalDevice,
                           vk::Queue graphicsQueue,
                           vk::CommandPool commandPool,
                           vk::Buffer srcBuffer,
                           vk::Buffer dstBuffer,
                           vk::DeviceSize size);

    static void uploadData(vk::Device logicalDevice,
                           vk::PhysicalDevice physicalDevice,
                           vk::Queue graphicsQueue,
                           vk::CommandPool commandPool,
                           vk::Buffer dstBuffer,
                           vk::DeviceSize size,
                           void* data);

    static uint32_t FindMemoryType(vk::PhysicalDevice physicalDevice,
                                   uint32_t typeFilter,
                                   vk::MemoryPropertyFlags properties);
};
}  // namespace ez
