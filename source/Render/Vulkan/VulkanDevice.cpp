#include "VulkanDevice.hpp"

#include <set>
#include <string>

#include <SDL2/SDL_vulkan.h>

#include "source/Render/Vulkan/Utils.hpp"
#include "source/Render/Vulkan/VulkanSwapchain.hpp"

using namespace Ride;

const int WIDTH = 800;
const int HEIGHT = 600;

const std::vector<const char*> requiredDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VulkanDevice::VulkanDevice(vk::Instance instanceIn)
    : instance(instanceIn)
{
    ready = InitWindow() && PickPhysicalDevice() && CreateLogicalDevice();
}

bool VulkanDevice::InitWindow()
{
    window = SDL_CreateWindow(
        "Vulkan Ride",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
    );

    SDL_bool result = SDL_Vulkan_CreateSurface(window, instance, &surface);

    return window != 0 && result == SDL_TRUE;
}

bool VulkanDevice::CheckDeviceExtensionSupport(vk::PhysicalDevice device) {
    auto extensionsRV = device.enumerateDeviceExtensionProperties();
    if (extensionsRV.result != vk::Result::eSuccess)
    {
        return false;
    }
    std::vector<vk::ExtensionProperties> availableExtensions = extensionsRV.value;

    std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool VulkanDevice::IsDeviceSuitable(vk::PhysicalDevice device) {
    QueueFamilyIndices indices = FindQueueFamilies(device, surface);

    bool extensionsSupported = CheckDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = VulkanSwapchain::QuerySwapChainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool VulkanDevice::PickPhysicalDevice()
{
    auto devicesRV = instance.enumeratePhysicalDevices();
    if (devicesRV.result != vk::Result::eSuccess)
    {
        printf("Failed to find GPUs with Vulkan support!");
        return false;
    }
    const std::vector<vk::PhysicalDevice>& devices = devicesRV.value;

    for (const auto& device : devices) {
        if (IsDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (!physicalDevice) {
        printf("Failed to find a suitable GPU!");
        return false;
    }
    return true;
}

bool VulkanDevice::CreateLogicalDevice() {
    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

    float queuePriority = 1.0f;
    for (int queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures = {};

    vk::DeviceCreateInfo createInfo = {};

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (physicalDevice.createDevice(&createInfo, nullptr, &logicalDevice) != vk::Result::eSuccess) {
        printf("Failed to create logical device!");
        return false;
    }

    logicalDevice.getQueue(indices.graphicsFamily, 0, &graphicsQueue);
    logicalDevice.getQueue(indices.presentFamily, 0, &presentQueue);
    return true;
}

VulkanDevice::~VulkanDevice()
{
    logicalDevice.destroy();
    instance.destroySurfaceKHR(surface);
    SDL_DestroyWindow(window);
}
