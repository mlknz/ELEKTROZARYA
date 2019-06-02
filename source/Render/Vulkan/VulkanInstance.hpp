#pragma once

#include <iostream>
#include <vector>

#include <Vulkan/Vulkan.hpp>

namespace Ride {

class VulkanInstance
{
public:
    VulkanInstance();
    ~VulkanInstance();

    bool Ready() const { return ready; }

    vk::Instance GetInstance() { return instance; }

private:
    bool CreateVulkanInstance();
    bool CheckValidationLayerSupport();

    bool SetupDebugCallback();

    std::vector<const char*> supportedExtensions;

    vk::Instance instance;
    VkDebugReportCallbackEXT vkDebugCallback;

    bool ready = false;
};

}
