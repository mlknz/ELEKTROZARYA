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
                 vk::CommandPool,
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
    vk::Queue GetGraphicsQueue() { return graphicsQueue; }
    vk::Queue GetPresentQueue() { return presentQueue; }

    vk::CommandPool GetGraphicsCommandPool() { return graphicsCommandPool; }
    vk::DescriptorPool GetDescriptorPool() { return descriptorPool; }

    static ResultValue<std::unique_ptr<VulkanDevice>> CreateVulkanDevice(vk::Instance instance);

   private:
    static ResultValue<vk::PhysicalDevice> PickPhysicalDevice(vk::Instance, vk::SurfaceKHR);

    static bool IsDeviceSuitable(vk::PhysicalDevice, vk::SurfaceKHR);
    static bool CheckDeviceExtensionSupport(vk::PhysicalDevice);

    static ResultValue<vk::Device> CreateDevice(vk::PhysicalDevice, const QueueFamilyIndices&);
    static ResultValue<vk::CommandPool> CreateGraphicsCommandPool(vk::Device,
                                                                  const QueueFamilyIndices&);
    static ResultValue<vk::DescriptorPool> CreateDescriptorPool(vk::Device device);

    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;

    SDL_Window* window;
    vk::SurfaceKHR surface;
    vk::Queue graphicsQueue;
    vk::Queue presentQueue;

    vk::CommandPool graphicsCommandPool;
    vk::DescriptorPool descriptorPool;

    QueueFamilyIndices queueFamilyIndices;
};

}  // namespace ez
