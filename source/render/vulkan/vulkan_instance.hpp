#pragma once

#include <iostream>
#include <vector>

#include "render/graphics_result.hpp"
#include "render/vulkan_include.hpp"

namespace ez
{
class VulkanInstance
{
   public:
    VulkanInstance() = delete;
    VulkanInstance(const VulkanInstance&) = delete;

    VulkanInstance(vk::Instance aInstance,
                   std::vector<const char*> aSupportedExtensions,
                   VkDebugReportCallbackEXT aVkDebugCallback);
    ~VulkanInstance();

    vk::Instance GetInstance() { return instance; }

    static ResultValue<std::unique_ptr<VulkanInstance>> CreateVulkanInstance();

   private:
    static bool CheckValidationLayerSupport();
    static bool SetupDebugCallback(vk::Instance instance,
                                   VkDebugReportCallbackEXT& vkDebugCallback);

    vk::Instance instance;
    std::vector<const char*> supportedExtensions;
    VkDebugReportCallbackEXT vkDebugCallback;
};

}  // namespace ez
