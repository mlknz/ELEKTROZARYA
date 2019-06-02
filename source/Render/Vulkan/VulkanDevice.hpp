#pragma once

#include <iostream>
#include <vector>

#include <SDL2/SDL.h>
#include <Vulkan/Vulkan.hpp>

namespace Ride {

class VulkanDevice
{
public:
    VulkanDevice(vk::Instance instance);
    ~VulkanDevice();

    bool Ready() const { return ready; }

    vk::Device GetDevice() { return logicalDevice; }
    vk::PhysicalDevice GetPhysicalDevice() { return physicalDevice; }
    vk::SurfaceKHR GetSurface() { return surface; }
    SDL_Window* GetWindow() { return window; }

    vk::Queue GetGraphicsQueue() { return graphicsQueue; }
    vk::Queue GetPresentQueue() { return presentQueue; }

private:
    bool InitWindow();
    bool PickPhysicalDevice();
    bool CreateLogicalDevice();

    bool IsDeviceSuitable(vk::PhysicalDevice);
    bool CheckDeviceExtensionSupport(vk::PhysicalDevice);

    SDL_Window* window = nullptr;

    vk::Instance instance;
    VkSurfaceKHR surface;
    vk::PhysicalDevice physicalDevice;
    vk::Device logicalDevice;

    vk::Queue graphicsQueue;
    vk::Queue presentQueue;

    bool ready = false;
};

}
