#pragma once

#include <vector>
#include <SDL2/SDL.h>
#include <Vulkan/Vulkan.hpp>
#include <GraphicsResult.hpp>

namespace Ride
{

struct VulkanSwapchainCreateInfo
{
    vk::Device logicalDevice;
    vk::PhysicalDevice physicalDevice;
    vk::SurfaceKHR surface;
    SDL_Window* window;
};

struct VulkanSwapchainInfo
{
    vk::SwapchainKHR swapchain;
    std::vector<vk::Image> images;
    vk::Format imageFormat;
    vk::Extent2D extent;
    std::vector<vk::ImageView> imageViews;
    std::vector<vk::Framebuffer> framebuffers;
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

class VulkanSwapchain
{
public:
    VulkanSwapchain() = delete;
    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain(const VulkanSwapchainCreateInfo& ci, VulkanSwapchainInfo&& info);
    ~VulkanSwapchain();

    VulkanSwapchainInfo& GetInfo() { return info; }

    void Cleanup();

    static ResultValue<std::unique_ptr<VulkanSwapchain>> CreateVulkanSwapchain(const VulkanSwapchainCreateInfo& ci);
    static ResultValue<SwapChainSupportDetails> QuerySwapchainSupport(vk::PhysicalDevice, vk::SurfaceKHR);

private:
    static ResultValue<VulkanSwapchainInfo> CreateSwapchain(const VulkanSwapchainCreateInfo& ci);

    static vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    static vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    static vk::Extent2D ChooseSwapExtent(SDL_Window* window, const vk::SurfaceCapabilitiesKHR& capabilities);

    static GraphicsResult CreateImageViews(vk::Device logicalDevice, VulkanSwapchainInfo& info);

    VulkanSwapchainInfo info;

    SDL_Window* window = nullptr;
    vk::SurfaceKHR surface;
    vk::PhysicalDevice physicalDevice;
    vk::Device logicalDevice;
};

}
