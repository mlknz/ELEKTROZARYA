#pragma once

#include <vector>

#include "render/graphics_result.hpp"
#include "render/vulkan_include.hpp"

namespace ez
{
const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

// todo: remove enableValidationLayers - move standard validation define to cmake?
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices final
{
    uint32_t graphicsFamily = std::numeric_limits<uint32_t>::max();
    uint32_t computeFamily = std::numeric_limits<uint32_t>::max();
    uint32_t presentFamily = std::numeric_limits<uint32_t>::max();

    bool IsComplete()
    {
        return graphicsFamily < std::numeric_limits<uint32_t>::max() &&
               computeFamily < std::numeric_limits<uint32_t>::max() &&
               presentFamily < std::numeric_limits<uint32_t>::max();
    }
};

inline QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    QueueFamilyIndices result;
    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueCount > 0)
        {
            if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
            {
                result.graphicsFamily = i;
            }
            if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute)
            {
                result.computeFamily = i;
            }
        }
        VkBool32 presentSupport = false;
        CheckVkResult(device.getSurfaceSupportKHR(i, surface, &presentSupport));

        if (queueFamily.queueCount > 0 && presentSupport) { result.presentFamily = i; }

        if (result.IsComplete()) { break; }

        i++;
    }

    return result;
}

}  // namespace ez
