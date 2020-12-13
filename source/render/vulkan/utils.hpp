#pragma once

#include <vector>
#include "render/vulkan_include.hpp"

namespace ez
{

const std::vector<const char*> validationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

// todo: remove enableValidationLayers
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
    int graphicsFamily = -1;
    int presentFamily = -1;

    bool isComplete() {
        return graphicsFamily >= 0 && presentFamily >= 0;
    }
};

inline QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    QueueFamilyIndices result;
    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            result.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        device.getSurfaceSupportKHR(static_cast<uint32_t>(i), surface, &presentSupport);

        if (queueFamily.queueCount > 0 && presentSupport) {
            result.presentFamily = i;
        }

        if (result.isComplete()) {
            break;
        }

        i++;
    }

    return result;
}

}
