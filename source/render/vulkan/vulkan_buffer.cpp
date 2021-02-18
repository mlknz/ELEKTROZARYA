#include "vulkan_buffer.hpp"

#include <stdio.h>

#include "core/log_assert.hpp"
#include "render/graphics_result.hpp"
#include "render/vulkan/vulkan_command_buffer.hpp"

namespace ez
{
uint32_t VulkanBuffer::FindMemoryType(vk::PhysicalDevice physicalDevice,
                                      uint32_t typeFilter,
                                      vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memProperties;
    physicalDevice.getMemoryProperties(&memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    EZASSERT(false, "Failed to find suitable memory type!");
    return 0;
}

bool VulkanBuffer::createBuffer(vk::Device logicalDevice,
                                vk::PhysicalDevice physicalDevice,
                                vk::DeviceSize size,
                                vk::BufferUsageFlags usage,
                                vk::MemoryPropertyFlags properties,
                                vk::Buffer& buffer,
                                vk::DeviceMemory& bufferMemory)
{
    vk::BufferCreateInfo bufferInfo = {};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    if (logicalDevice.createBuffer(&bufferInfo, nullptr, &buffer) != vk::Result::eSuccess)
    {
        EZLOG("failed to create buffer!");
        return false;
    }

    vk::MemoryRequirements memRequirements;
    logicalDevice.getBufferMemoryRequirements(buffer, &memRequirements);

    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    if (logicalDevice.allocateMemory(&allocInfo, nullptr, &bufferMemory) !=
        vk::Result::eSuccess)
    {
        EZLOG("failed to allocate buffer memory!");
        return false;
    }

    CheckVkResult(logicalDevice.bindBufferMemory(buffer, bufferMemory, 0));
    return true;
}

void VulkanBuffer::copyBuffer(vk::Device logicalDevice,
                              vk::Queue graphicsQueue,
                              vk::CommandPool commandPool,
                              vk::Buffer srcBuffer,
                              vk::Buffer dstBuffer,
                              vk::DeviceSize size)
{
    VulkanOneTimeCommandBuffer oneTimeCB =
        VulkanOneTimeCommandBuffer::Start(logicalDevice, commandPool);

    vk::BufferCopy copyRegion{ 0, 0, size };
    oneTimeCB.GetCommandBuffer().copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

    oneTimeCB.EndSubmitAndWait(graphicsQueue);
}

void VulkanBuffer::uploadData(vk::Device logicalDevice,
                              vk::PhysicalDevice physicalDevice,
                              vk::Queue graphicsQueue,
                              vk::CommandPool commandPool,
                              vk::Buffer dstBuffer,
                              vk::DeviceSize size,
                              void* bufferData)
{
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    VulkanBuffer::createBuffer(
        logicalDevice,
        physicalDevice,
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory);

    void* data;

    CheckVkResult(
        logicalDevice.mapMemory(stagingBufferMemory, 0, size, vk::MemoryMapFlags(), &data));
    memcpy(data, bufferData, static_cast<size_t>(size));
    logicalDevice.unmapMemory(stagingBufferMemory);

    VulkanBuffer::copyBuffer(
        logicalDevice, graphicsQueue, commandPool, stagingBuffer, dstBuffer, size);

    logicalDevice.destroyBuffer(stagingBuffer);
    logicalDevice.freeMemory(stagingBufferMemory);
}

}  // namespace ez
