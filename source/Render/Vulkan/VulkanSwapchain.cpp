#include "VulkanSwapchain.hpp"

#include <algorithm>
#include <SDL2/SDL_vulkan.h>
#include "source/Render/Vulkan/Utils.hpp"

using namespace Ride;

SwapChainSupportDetails VulkanSwapchain::QuerySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface) {

    SwapChainSupportDetails details;
    device.getSurfaceCapabilitiesKHR(surface, &details.capabilities);

    auto formatsRV = device.getSurfaceFormatsKHR(surface);
    if (formatsRV.result == vk::Result::eSuccess)
    {
        details.formats = formatsRV.value;
    }
    else
    {
        printf("device.getSurfaceFormatsKHR failed");
    }

    auto presentModesRV = device.getSurfacePresentModesKHR(surface);
    if (presentModesRV.result == vk::Result::eSuccess)
    {
        details.presentModes = presentModesRV.value;
    }
    else
    {
        printf("device.getSurfacePresentModesKHR failed");
    }

    return details;
}

VulkanSwapchain::VulkanSwapchain(vk::Device logicalDevice, vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, SDL_Window* window)
    : logicalDevice(logicalDevice)
    , physicalDevice(physicalDevice)
    , surface(surface)
    , window(window)
{
    ready = CreateSwapchain() && CreateImageViews();
}

bool VulkanSwapchain::CreateSwapchain()
{
    info = {};
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice, surface);

    vk::SurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    vk::Extent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo = {};
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);
    uint32_t queueFamilyIndices[] = {(uint32_t) indices.graphicsFamily, (uint32_t) indices.presentFamily};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (logicalDevice.createSwapchainKHR(&createInfo, nullptr, &info.swapchain) != vk::Result::eSuccess) {
        printf("failed to create swap chain!");
        return false;
    }

    vkGetSwapchainImagesKHR(logicalDevice, info.swapchain, &imageCount, nullptr);
    info.images.resize(imageCount);
    logicalDevice.getSwapchainImagesKHR(info.swapchain, &imageCount, info.images.data());

    info.imageFormat = surfaceFormat.format;
    info.extent = extent;

    return true;
}

bool VulkanSwapchain::CreateImageViews() {
    info.imageViews.resize(info.images.size());

    for (size_t i = 0; i < info.images.size(); i++) {
        vk::ImageViewCreateInfo createInfo = {};
        createInfo.image = info.images[i];
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.format = info.imageFormat;
        createInfo.components.r = vk::ComponentSwizzle::eIdentity;
        createInfo.components.g = vk::ComponentSwizzle::eIdentity;
        createInfo.components.b = vk::ComponentSwizzle::eIdentity;
        createInfo.components.a = vk::ComponentSwizzle::eIdentity;
        createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (logicalDevice.createImageView(&createInfo, nullptr, &info.imageViews[i]) != vk::Result::eSuccess) {
            printf("failed to create image views!");
            return false;
        }
    }
    return true;
}

vk::SurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined) {
        return {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
    }

    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR VulkanSwapchain::ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        } else if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
            bestMode = availablePresentMode;
        }
    }

    return bestMode;
}

vk::Extent2D VulkanSwapchain::ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        SDL_Vulkan_GetDrawableSize(window, &width, &height);

        vk::Extent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

VulkanSwapchain::~VulkanSwapchain()
{
    Cleanup();
}

void VulkanSwapchain::Cleanup()
{
    for (size_t i = 0; i < info.framebuffers.size(); i++) {
        logicalDevice.destroyFramebuffer(info.framebuffers[i], nullptr);
    }

    for (size_t i = 0; i < info.imageViews.size(); i++) {
        logicalDevice.destroyImageView(info.imageViews[i], nullptr);
    }

    logicalDevice.destroySwapchainKHR(info.swapchain);
    info = {};
}
