#pragma once

#include <iostream>
#include <vector>

#include <SDL.h>
#include "Vulkan.hpp"
#include "GraphicsResult.hpp"

namespace Ride
{

class VulkanDevice
{
public:
    VulkanDevice() = delete;
    VulkanDevice(const VulkanDevice&) = delete;

    VulkanDevice(vk::Instance, vk::PhysicalDevice, vk::Device, vk::CommandPool, vk::DescriptorPool, SDL_Window*, VkSurfaceKHR);
    ~VulkanDevice();

    vk::Device GetDevice() { return device; }
    vk::PhysicalDevice GetPhysicalDevice() { return physicalDevice; }
    vk::SurfaceKHR GetSurface() { return surface; }
    SDL_Window* GetWindow() { return window; }

    vk::Queue GetGraphicsQueue() { return graphicsQueue; }
    vk::Queue GetPresentQueue() { return presentQueue; }
    vk::CommandPool GetGraphicsCommandPool() { return graphicsCommandPool; }
    vk::DescriptorPool GetDescriptorPool() { return descriptorPool; }

    static ResultValue<std::unique_ptr<VulkanDevice>> CreateVulkanDevice(vk::Instance instance);

private:
    static ResultValue<vk::PhysicalDevice> PickPhysicalDevice(vk::Instance, VkSurfaceKHR);

    static bool IsDeviceSuitable(vk::PhysicalDevice, VkSurfaceKHR);
    static bool CheckDeviceExtensionSupport(vk::PhysicalDevice);

    static ResultValue<vk::Device> CreateDevice(vk::PhysicalDevice, VkSurfaceKHR);
    static ResultValue<vk::CommandPool> CreateGraphicsCommandPool(vk::PhysicalDevice, vk::Device device, VkSurfaceKHR);
    static ResultValue<vk::DescriptorPool> CreateDescriptorPool(vk::Device device);

    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;

    SDL_Window* window;
    VkSurfaceKHR surface;
    vk::Queue graphicsQueue;
    vk::Queue presentQueue;

    vk::CommandPool graphicsCommandPool;
    vk::DescriptorPool descriptorPool;
};

}
