#pragma once

#include <graphics_result.hpp>
#include <vulkan_include.hpp>

namespace Ride
{

class VulkanDeviceMemoryManager
{
public:
    VulkanDeviceMemoryManager() = delete;
    VulkanDeviceMemoryManager(const VulkanDeviceMemoryManager&) = delete;
    VulkanDeviceMemoryManager(vk::Device aLogicalDevice, vk::PhysicalDevice aPhysicalDevice);

    ~VulkanDeviceMemoryManager();

    vk::Buffer GetVertexBuffer() { return vertexBuffer; }
    vk::Buffer GetIndexBuffer() { return indexBuffer; }
    vk::Buffer GetUniformBuffer() { return uniformBuffer; }

    vk::DeviceMemory GetUniformBufferMemory() { return uniformBufferMemory; } // todo: remove

    static ResultValue<std::unique_ptr<VulkanDeviceMemoryManager>> CreateVulkanDeviceMemoryManager(vk::Device aLogicalDevice, vk::PhysicalDevice aPhysicalDevice);

private:
    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexBufferMemory;
    uint32_t vertexBufferSize = 1000; // todo: move these values to some config

    vk::Buffer indexBuffer;
    vk::DeviceMemory indexBufferMemory;
    uint32_t indexBufferSize = 200;

    vk::Buffer uniformBuffer;
    vk::DeviceMemory uniformBufferMemory;
    uint32_t uniformBufferSize = 200;

    vk::Device logicalDevice;
    vk::PhysicalDevice physicalDevice;
};

}
