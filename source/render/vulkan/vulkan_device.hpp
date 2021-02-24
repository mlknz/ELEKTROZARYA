#pragma once

#include <SDL.h>

#include <iostream>
#include <vector>

#include "render/graphics_result.hpp"
#include "render/vulkan/utils.hpp"
#include "render/vulkan_include.hpp"

namespace ez
{
class VulkanDevice
{
   public:
    VulkanDevice() = delete;
    VulkanDevice(const VulkanDevice&) = delete;

    VulkanDevice(vk::Instance,
                 vk::PhysicalDevice,
                 vk::Device,
                 vk::CommandPool graphicsCommandPool,
                 vk::CommandPool computeCommandPool,
                 vk::DescriptorPool,
                 SDL_Window*,
                 vk::SurfaceKHR,
                 QueueFamilyIndices);
    ~VulkanDevice();

    vk::Device GetDevice() { return device; }
    vk::PhysicalDevice GetPhysicalDevice() { return physicalDevice; }
    vk::SurfaceKHR GetSurface() { return surface; }
    SDL_Window* GetWindow() { return window; }

    const QueueFamilyIndices& GetQueueFamilyIndices() const { return queueFamilyIndices; }
    vk::Queue GetGraphicsQueue() const { return graphicsQueue; }
    vk::Queue GetComputeQueue() const { return computeQueue; }
    vk::Queue GetPresentQueue() const { return presentQueue; }

    vk::CommandPool GetGraphicsCommandPool() const { return graphicsCommandPool; }
    vk::CommandPool GetComputeCommandPool() const { return computeCommandPool; }
    vk::DescriptorPool GetDescriptorPool() const { return descriptorPool; }

    bool IsMSAA8xSupported() const { return msaa8xSupported; }

    static ResultValue<std::unique_ptr<VulkanDevice>> CreateVulkanDevice(vk::Instance instance);

   private:
    static ResultValue<vk::PhysicalDevice> PickPhysicalDevice(vk::Instance, vk::SurfaceKHR);

    static bool IsDeviceSuitable(vk::PhysicalDevice, vk::SurfaceKHR);
    static bool CheckDeviceExtensionSupport(vk::PhysicalDevice);

    static ResultValue<vk::Device> CreateDevice(vk::PhysicalDevice, const QueueFamilyIndices&);
    static ResultValue<vk::CommandPool> CreateCommandPool(vk::Device,
                                                          uint32_t queueFamilyIndex);
    static ResultValue<vk::DescriptorPool> CreateDescriptorPool(vk::Device device);

    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;

    SDL_Window* window;
    vk::SurfaceKHR surface;
    vk::Queue graphicsQueue;
    vk::Queue computeQueue;
    vk::Queue presentQueue;

    vk::CommandPool graphicsCommandPool;
    vk::CommandPool computeCommandPool;
    vk::DescriptorPool descriptorPool;

    QueueFamilyIndices queueFamilyIndices;

    bool msaa8xSupported = false;
};

}  // namespace ez
