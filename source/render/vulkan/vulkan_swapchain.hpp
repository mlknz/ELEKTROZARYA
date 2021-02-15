#pragma once

#include <SDL.h>

#include <vector>

#include "render/graphics_result.hpp"
#include "render/vulkan/utils.hpp"
#include "render/vulkan_include.hpp"

namespace ez
{
struct VulkanSwapchainCreateInfo final
{
    vk::Device logicalDevice;
    vk::PhysicalDevice physicalDevice;
    vk::SurfaceKHR surface;
    SDL_Window* window;
    const QueueFamilyIndices& queueFamilyIndices;
    bool msaa8xEnabled;
};

struct VulkanSwapchainInfo final
{
    vk::SwapchainKHR swapchain;
    std::vector<vk::Image> images;
    vk::Format imageFormat;
    vk::Extent2D extent;
    std::vector<vk::ImageView> imageViews;
    std::vector<vk::Framebuffer> framebuffers;

    vk::Image depthImage;
    vk::DeviceMemory depthImageMemory;
    vk::ImageView depthImageView;

    bool msaa8xEnabled;
};

struct SwapChainSupportDetails final
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

class VulkanSwapchain final
{
   public:
    VulkanSwapchain() = delete;
    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain(const VulkanSwapchainCreateInfo& ci, VulkanSwapchainInfo&& info);
    ~VulkanSwapchain();

    VulkanSwapchainInfo& GetInfo() { return info; }
    GraphicsResult CreateFramebuffersForRenderPass(vk::RenderPass vkRenderPass);

    static ResultValue<std::unique_ptr<VulkanSwapchain>> CreateVulkanSwapchain(
        const VulkanSwapchainCreateInfo& ci);
    static ResultValue<SwapChainSupportDetails> QuerySwapchainSupport(vk::PhysicalDevice,
                                                                      vk::SurfaceKHR);

   private:
    static ResultValue<VulkanSwapchainInfo> CreateSwapchain(
        const VulkanSwapchainCreateInfo& ci);

    static vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(
        const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    static vk::PresentModeKHR ChooseSwapPresentMode(
        const std::vector<vk::PresentModeKHR>& availablePresentModes);
    static vk::Extent2D ChooseSwapExtent(SDL_Window* window,
                                         const vk::SurfaceCapabilitiesKHR& capabilities);

    static GraphicsResult CreateImageViews(vk::Device logicalDevice, VulkanSwapchainInfo& info);

    VulkanSwapchainInfo info;

    vk::Device logicalDevice;  // todo: store just VulkanSwapchainCreateInfo ?
    vk::PhysicalDevice physicalDevice;
    vk::SurfaceKHR surface;
    SDL_Window* window = nullptr;
};

}  // namespace ez
