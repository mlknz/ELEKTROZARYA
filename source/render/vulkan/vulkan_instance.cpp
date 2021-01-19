#include "vulkan_instance.hpp"

#include <cstring>
#include "render/vulkan/utils.hpp"
#include "core/log_assert.hpp"

using namespace ez;

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
    [[maybe_unused]] VkDebugReportFlagsEXT flags,
    [[maybe_unused]] VkDebugReportObjectTypeEXT objType,
    [[maybe_unused]] uint64_t obj,
    [[maybe_unused]] size_t location,
    [[maybe_unused]] int32_t code,
    [[maybe_unused]] const char* layerPrefix,
    const char* msg,
    [[maybe_unused]] void* userData)
{
    printf("VULKAN VALIDATION: %s\n", msg);
    return VK_FALSE;
}

VkResult CreateDebugReportCallbackEXT(vk::Instance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
    auto func = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance.operator VkInstance(), "vkCreateDebugReportCallbackEXT"));
    if (func != nullptr) {
        return func(instance.operator VkInstance(), pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

bool VulkanInstance::SetupDebugCallback(vk::Instance instance, VkDebugReportCallbackEXT& vkDebugCallback) {
    if (!enableValidationLayers) return true;

    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = VulkanDebugCallback;

    if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &vkDebugCallback) != VK_SUCCESS) {
        EZLOG("failed to set up debug callback!");
        return false;
    }

    return true;
}

VulkanInstance::VulkanInstance(vk::Instance aInstance, std::vector<const char*> aSupportedExtensions, VkDebugReportCallbackEXT aVkDebugCallback)
    : instance(std::move(aInstance)),
      supportedExtensions(std::move(aSupportedExtensions)),
      vkDebugCallback(std::move(aVkDebugCallback))
{}

ResultValue<std::unique_ptr<VulkanInstance>> VulkanInstance::CreateVulkanInstance()
{
    if (enableValidationLayers && !CheckValidationLayerSupport()) {
        EZLOG("Error: validation layers requested, but not available!");
        return GraphicsResult::Error;
    }

    vk::ApplicationInfo appInfo = {};
    appInfo.pApplicationName = "ELEKTROZARYA Vulkan C++ Sandbox";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "ELEKTROZARYA Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);

    auto extPropsResultValue = vk::enumerateInstanceExtensionProperties();

    std::vector<vk::ExtensionProperties> extensionsProps;
    if (extPropsResultValue.result == vk::Result::eSuccess)
    {
        extensionsProps = extPropsResultValue.value;
    }

    vk::Instance instance;
    std::vector<const char*> supportedExtensions;
    VkDebugReportCallbackEXT vkDebugCallback;

    for (const auto& extension : extensionsProps) {
        supportedExtensions.push_back(extension.extensionName);
    }

    vk::InstanceCreateInfo createInfo = {};
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(supportedExtensions.size());
    createInfo.ppEnabledExtensionNames = supportedExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vk::createInstance(&createInfo, nullptr, &instance) != vk::Result::eSuccess)
    {
        EZLOG("Can't create vk instance");
        return GraphicsResult::Error;
    }

    if (!SetupDebugCallback(instance, vkDebugCallback))
    {
        EZLOG("Can't setup debug callbacks");
        return GraphicsResult::Error;
    }

    return {GraphicsResult::Ok, std::make_unique<VulkanInstance>(std::move(instance), std::move(supportedExtensions), std::move(vkDebugCallback))};
}

void DestroyDebugReportCallbackEXT(vk::Instance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
    auto func = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance.operator VkInstance(), "vkDestroyDebugReportCallbackEXT"));
    if (func != nullptr) {
        func(instance.operator VkInstance(), callback, pAllocator);
    }
}

VulkanInstance::~VulkanInstance()
{
    if (vkDebugCallback)
    {
        DestroyDebugReportCallbackEXT(instance, vkDebugCallback, nullptr);
    }

    instance.destroy();
}

bool VulkanInstance::CheckValidationLayerSupport() {

    auto layersResultValue = vk::enumerateInstanceLayerProperties();
    std::vector<vk::LayerProperties> availableLayers;
    if (layersResultValue.result == vk::Result::eSuccess)
    {
        availableLayers = layersResultValue.value;
    }

    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}
