#pragma once

#include <Vulkan/Vulkan.hpp>

namespace Ride
{
class VulkanBuffer
{
public:
    static bool createBuffer(
            VkDevice logicalDevice,
            VkPhysicalDevice physicalDevice,
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer& buffer,
            VkDeviceMemory& bufferMemory);

    static void copyBuffer(
            VkDevice logicalDevice,
            VkQueue graphicsQueue,
            VkCommandPool commandPool,
            VkBuffer srcBuffer,
            VkBuffer dstBuffer,
            VkDeviceSize size);
};
}
