#include "vulkan_buffer.hpp"

#include <stdio.h>
#include <cassert>

namespace ez {

uint32_t findMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties;
    physicalDevice.getMemoryProperties(&memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    assert("Failed to find suitable memory type!" && false);
    return 0;
}

bool VulkanBuffer::createBuffer(vk::Device logicalDevice, vk::PhysicalDevice physicalDevice, vk::DeviceSize size,
                  vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory) {
    vk::BufferCreateInfo bufferInfo = {};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    if (logicalDevice.createBuffer(&bufferInfo, nullptr, &buffer) != vk::Result::eSuccess) {
        printf("failed to create buffer!");
        return false;
    }

    vk::MemoryRequirements memRequirements;
    logicalDevice.getBufferMemoryRequirements(buffer, &memRequirements);

    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    if (logicalDevice.allocateMemory(&allocInfo, nullptr, &bufferMemory) != vk::Result::eSuccess) {
        printf("failed to allocate buffer memory!");
        return false;
    }

    logicalDevice.bindBufferMemory(buffer, bufferMemory, 0);
    return true;
}

void VulkanBuffer::copyBuffer(vk::Device logicalDevice, vk::Queue graphicsQueue, vk::CommandPool commandPool,
                              vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
    vk::CommandBufferAllocateInfo allocInfo = {};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer;
    logicalDevice.allocateCommandBuffers(&allocInfo, &commandBuffer);

    vk::CommandBufferBeginInfo beginInfo = {};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    commandBuffer.begin(&beginInfo);

    vk::BufferCopy copyRegion = {};
    copyRegion.size = size;
    copyRegion.dstOffset = 0; // todo
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

    commandBuffer.end();

    vk::SubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    graphicsQueue.submit(1, &submitInfo, nullptr);
    graphicsQueue.waitIdle();

    logicalDevice.freeCommandBuffers(commandPool, 1, &commandBuffer);
}

}
