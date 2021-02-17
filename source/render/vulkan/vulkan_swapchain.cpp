#include "vulkan_swapchain.hpp"

#include <SDL_vulkan.h>

#include <algorithm>

#include "core/log_assert.hpp"
#include "render/config.hpp"
#include "render/graphics_result.hpp"
#include "render/vulkan/vulkan_buffer.hpp"
#include "render/vulkan/vulkan_image.hpp"

using namespace ez;

VulkanSwapchain::VulkanSwapchain(const VulkanSwapchainCreateInfo& ci,
                                 VulkanSwapchainInfo&& info)
    : info(std::move(info))
    , logicalDevice(ci.logicalDevice)
    , physicalDevice(ci.physicalDevice)
    , surface(ci.surface)
    , window(ci.window)
{
}

ResultValue<SwapChainSupportDetails> VulkanSwapchain::QuerySwapchainSupport(
    vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    SwapChainSupportDetails details;
    CheckVkResult(device.getSurfaceCapabilitiesKHR(surface, &details.capabilities));

    auto formatsRV = device.getSurfaceFormatsKHR(surface);
    if (formatsRV.result != vk::Result::eSuccess)
    {
        EZLOG("device.getSurfaceFormatsKHR failed");
        return { GraphicsResult::Error, details };
    }

    details.formats = formatsRV.value;

    auto presentModesRV = device.getSurfacePresentModesKHR(surface);
    if (presentModesRV.result != vk::Result::eSuccess)
    {
        EZLOG("device.getSurfacePresentModesKHR failed");
        return { GraphicsResult::Error, details };
    }
    details.presentModes = presentModesRV.value;

    return { GraphicsResult::Ok, details };
}

ResultValue<std::unique_ptr<VulkanSwapchain>> VulkanSwapchain::CreateVulkanSwapchain(
    const VulkanSwapchainCreateInfo& ci)
{
    ResultValue<VulkanSwapchainInfo> vulkanSwapchainInfo = CreateSwapchain(ci);
    if (vulkanSwapchainInfo.result != GraphicsResult::Ok) { return vulkanSwapchainInfo.result; }
    VulkanSwapchainInfo& swapchainInfo = vulkanSwapchainInfo.value;

    vk::SampleCountFlagBits samplesCount =
        swapchainInfo.msaa8xEnabled ? vk::SampleCountFlagBits::e8 : vk::SampleCountFlagBits::e1;
    if (swapchainInfo.msaa8xEnabled)
    {
        ResultValue<ImageWithMemory> multisampledImageRV =
            Image::CreateImage2DWithMemory(ci.logicalDevice,
                                           ci.physicalDevice,
                                           swapchainInfo.imageFormat,
                                           vk::ImageUsageFlagBits::eColorAttachment |
                                               vk::ImageUsageFlagBits::eTransientAttachment,
                                           1,
                                           swapchainInfo.extent.width,
                                           swapchainInfo.extent.height,
                                           samplesCount);
        if (multisampledImageRV.result != GraphicsResult::Ok)
        {
            return multisampledImageRV.result;
        }
        swapchainInfo.multisampledImage = multisampledImageRV.value.image;
        swapchainInfo.multisampledImageMemory = multisampledImageRV.value.imageMemory;
    }

    ResultValue<ImageWithMemory> depthImageRV =
        Image::CreateImage2DWithMemory(ci.logicalDevice,
                                       ci.physicalDevice,
                                       Config::DepthAttachmentFormat,
                                       vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                       1,
                                       swapchainInfo.extent.width,
                                       swapchainInfo.extent.height,
                                       samplesCount);
    if (depthImageRV.result != GraphicsResult::Ok) { return depthImageRV.result; }
    swapchainInfo.depthImage = depthImageRV.value.image;
    swapchainInfo.depthImageMemory = depthImageRV.value.imageMemory;

    GraphicsResult imageViewsResult = CreateImageViews(ci.logicalDevice, swapchainInfo);
    if (imageViewsResult != GraphicsResult::Ok) { return imageViewsResult; }

    return { GraphicsResult::Ok,
             std::make_unique<VulkanSwapchain>(ci, std::move(swapchainInfo)) };
}

ResultValue<VulkanSwapchainInfo> VulkanSwapchain::CreateSwapchain(
    const VulkanSwapchainCreateInfo& ci)
{
    VulkanSwapchainInfo info = {};
    info.msaa8xEnabled = ci.msaa8xEnabled;
    ResultValue<SwapChainSupportDetails> swapchainSupportResult =
        QuerySwapchainSupport(ci.physicalDevice, ci.surface);
    if (swapchainSupportResult.result != GraphicsResult::Ok)
    {
        return swapchainSupportResult.result;
    }
    SwapChainSupportDetails swapchainSupport = std::move(swapchainSupportResult.value);

    vk::SurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapchainSupport.formats);
    vk::PresentModeKHR presentMode = ChooseSwapPresentMode(swapchainSupport.presentModes);
    vk::Extent2D extent = ChooseSwapExtent(ci.window, swapchainSupport.capabilities);

    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapchainSupport.capabilities.maxImageCount)
    {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo = {};
    createInfo.surface = ci.surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    uint32_t queueFamilyIndices[] = { ci.queueFamilyIndices.graphicsFamily,
                                      ci.queueFamilyIndices.presentFamily };

    if (ci.queueFamilyIndices.graphicsFamily != ci.queueFamilyIndices.presentFamily)
    {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (ci.logicalDevice.createSwapchainKHR(&createInfo, nullptr, &info.swapchain) !=
        vk::Result::eSuccess)
    {
        EZLOG("Failed to create swapchain!");
        return GraphicsResult::Error;
    }

    vkGetSwapchainImagesKHR(ci.logicalDevice.operator VkDevice(),
                            info.swapchain.operator VkSwapchainKHR(),
                            &imageCount,
                            nullptr);
    info.images.resize(imageCount);
    CheckVkResult(ci.logicalDevice.getSwapchainImagesKHR(
        info.swapchain, &imageCount, info.images.data()));

    info.imageFormat = surfaceFormat.format;
    info.extent = extent;

    return { GraphicsResult::Ok, info };
}

vk::SurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
    if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined)
    {
        return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
    }

    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == vk::Format::eB8G8R8A8Unorm &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR VulkanSwapchain::ChooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
    vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;

    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox)
        {
            return availablePresentMode;
        }
        else if (availablePresentMode == vk::PresentModeKHR::eImmediate)
        {
            bestMode = availablePresentMode;
        }
    }

    return bestMode;
}

vk::Extent2D VulkanSwapchain::ChooseSwapExtent(SDL_Window* window,
                                               const vk::SurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    int width, height;
    SDL_Vulkan_GetDrawableSize(window, &width, &height);

    vk::Extent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

    actualExtent.width =
        std::max(capabilities.minImageExtent.width,
                 std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height =
        std::max(capabilities.minImageExtent.height,
                 std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
}

GraphicsResult VulkanSwapchain::CreateImageViews(vk::Device logicalDevice,
                                                 VulkanSwapchainInfo& info)
{
    info.imageViews.resize(info.images.size());

    for (size_t i = 0; i < info.images.size(); i++)
    {
        ResultValue<vk::ImageView> imageViewRV =
            Image::CreateImageView2D(logicalDevice,
                                     info.images[i],
                                     info.imageFormat,
                                     vk::ImageAspectFlagBits::eColor,
                                     1);
        if (imageViewRV.result != GraphicsResult::Ok) { return imageViewRV.result; }
        info.imageViews[i] = imageViewRV.value;
    }

    if (info.multisampledImage)
    {
        ResultValue<vk::ImageView> multisampledImageViewRV =
            Image::CreateImageView2D(logicalDevice,
                                     info.multisampledImage,
                                     info.imageFormat,
                                     vk::ImageAspectFlagBits::eColor,
                                     1);
        if (multisampledImageViewRV.result != GraphicsResult::Ok)
        {
            return multisampledImageViewRV.result;
        }
        info.multisampledImageView = multisampledImageViewRV.value;
    }

    ResultValue<vk::ImageView> depthImageViewRV =
        Image::CreateImageView2D(logicalDevice,
                                 info.depthImage,
                                 Config::DepthAttachmentFormat,
                                 vk::ImageAspectFlagBits::eDepth,
                                 1);
    if (depthImageViewRV.result != GraphicsResult::Ok) { return depthImageViewRV.result; }
    info.depthImageView = depthImageViewRV.value;

    return GraphicsResult::Ok;
}

GraphicsResult VulkanSwapchain::CreateFramebuffersForRenderPass(vk::RenderPass vkRenderPass)
{
    info.framebuffers.resize(info.imageViews.size());

    for (size_t i = 0; i < info.imageViews.size(); i++)
    {
        std::array<vk::ImageView, 3> attachments = { info.imageViews[i],
                                                     info.depthImageView,
                                                     {} };
        if (Config::msaa8xEnabled)
        {
            attachments = { info.multisampledImageView,
                            info.depthImageView,
                            info.imageViews[i] };
        }

        vk::FramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.renderPass = vkRenderPass;
        framebufferInfo.attachmentCount = Config::msaa8xEnabled ? 3 : 2;
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = info.extent.width;
        framebufferInfo.height = info.extent.height;
        framebufferInfo.layers = 1;

        if (logicalDevice.createFramebuffer(&framebufferInfo, nullptr, &info.framebuffers[i]) !=
            vk::Result::eSuccess)
        {
            EZLOG("Failed to create framebuffer!");
            return GraphicsResult::Error;
        }
    }
    return GraphicsResult::Ok;
}

VulkanSwapchain::~VulkanSwapchain()
{
    for (size_t i = 0; i < info.framebuffers.size(); ++i)
    {
        logicalDevice.destroyFramebuffer(info.framebuffers[i], nullptr);
    }

    for (size_t i = 0; i < info.imageViews.size(); ++i)
    {
        logicalDevice.destroyImageView(info.imageViews[i], nullptr);
    }

    if (info.multisampledImage)
    {
        logicalDevice.destroyImageView(info.multisampledImageView, nullptr);
        logicalDevice.destroyImage(info.multisampledImage);
        logicalDevice.freeMemory(info.multisampledImageMemory);
    }

    logicalDevice.destroyImageView(info.depthImageView, nullptr);
    logicalDevice.destroyImage(info.depthImage);
    logicalDevice.freeMemory(info.depthImageMemory);

    logicalDevice.destroySwapchainKHR(info.swapchain);
    info = {};
}
