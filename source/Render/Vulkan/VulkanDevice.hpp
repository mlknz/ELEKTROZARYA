#pragma once

#include <iostream>
#include <vector>

#include <SDL2/SDL.h>
#include <Vulkan/Vulkan.hpp>
#include <GraphicsResult.hpp>

namespace Ride {

class VulkanDevice
{
public:
    VulkanDevice() = delete;
    VulkanDevice(const VulkanDevice&) = delete;

    VulkanDevice(vk::Instance, vk::PhysicalDevice, vk::Device, SDL_Window*, VkSurfaceKHR);
    ~VulkanDevice();

    vk::Device GetDevice() { return device; }
    vk::PhysicalDevice GetPhysicalDevice() { return physicalDevice; }
    vk::SurfaceKHR GetSurface() { return surface; }
    SDL_Window* GetWindow() { return window; }

    vk::Queue GetGraphicsQueue() { return graphicsQueue; }
    vk::Queue GetPresentQueue() { return presentQueue; }

    static ResultValue<std::unique_ptr<VulkanDevice>> CreateVulkanDevice(vk::Instance instance);

private:
    static ResultValue<vk::PhysicalDevice> PickPhysicalDevice(vk::Instance, VkSurfaceKHR);

    static bool IsDeviceSuitable(vk::PhysicalDevice, VkSurfaceKHR);
    static bool CheckDeviceExtensionSupport(vk::PhysicalDevice);

    static ResultValue<vk::Device> CreateDevice(vk::PhysicalDevice, VkSurfaceKHR);

    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;

    SDL_Window* window;
    VkSurfaceKHR surface;
    vk::Queue graphicsQueue;
    vk::Queue presentQueue;
};

}
