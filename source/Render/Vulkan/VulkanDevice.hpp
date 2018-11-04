#pragma once

#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>

namespace Ride {

class VulkanDevice
{
public:
    VulkanDevice(VkInstance instance);
    ~VulkanDevice();

    bool Ready() const { return ready; }

    VkDevice GetDevice() { return logicalDevice; }
    VkPhysicalDevice GetPhysicalDevice() { return physicalDevice; }
    VkSurfaceKHR GetSurface() { return surface; }
    SDL_Window* GetWindow() { return window; }

    VkQueue GetGraphicsQueue() { return graphicsQueue; }
    VkQueue GetPresentQueue() { return presentQueue; }

private:
    bool InitWindow();
    bool PickPhysicalDevice();
    bool CreateLogicalDevice();

    bool IsDeviceSuitable(VkPhysicalDevice);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice);

    VkInstance instance;

    SDL_Window* window = nullptr;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice = VK_NULL_HANDLE;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    bool ready = false;
};

}
