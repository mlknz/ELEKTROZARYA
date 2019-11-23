#include "vulkan_device_memory_manager.hpp"

#include <render/vulkan/vulkan_buffer.hpp>

using namespace Ride;

ResultValue<std::unique_ptr<VulkanDeviceMemoryManager>> VulkanDeviceMemoryManager::CreateVulkanDeviceMemoryManager(vk::Device aLogicalDevice, vk::PhysicalDevice aPhysicalDevice)
{
    return {GraphicsResult::Ok, std::make_unique<VulkanDeviceMemoryManager>(aLogicalDevice, aPhysicalDevice)};
}

VulkanDeviceMemoryManager::VulkanDeviceMemoryManager(vk::Device aLogicalDevice, vk::PhysicalDevice aPhysicalDevice)
    : logicalDevice(aLogicalDevice)
    , physicalDevice(aPhysicalDevice)
{
    VulkanBuffer::createBuffer(logicalDevice, physicalDevice, vertexBufferSize,
                               vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                               vk::MemoryPropertyFlagBits::eDeviceLocal,
                               vertexBuffer, vertexBufferMemory);
    VulkanBuffer::createBuffer(logicalDevice, physicalDevice, indexBufferSize,
                               vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                               vk::MemoryPropertyFlagBits::eDeviceLocal,
                               indexBuffer, indexBufferMemory);
    VulkanBuffer::createBuffer(logicalDevice, physicalDevice, uniformBufferSize,
                               vk::BufferUsageFlagBits::eUniformBuffer,
                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                               uniformBuffer, uniformBufferMemory);
}

VulkanDeviceMemoryManager::~VulkanDeviceMemoryManager()
{
    logicalDevice.destroyBuffer(uniformBuffer);
    logicalDevice.freeMemory(uniformBufferMemory);

    logicalDevice.destroyBuffer(indexBuffer);
    logicalDevice.freeMemory(indexBufferMemory);

    logicalDevice.destroyBuffer(vertexBuffer);
    logicalDevice.freeMemory(vertexBufferMemory);
}

