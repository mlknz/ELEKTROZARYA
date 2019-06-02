#pragma once

#include <vector>
#include <SDL2/SDL.h>
#include <Vulkan/Vulkan.hpp>

namespace Ride
{

struct SwapchainInfo
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
    static SwapChainSupportDetails QuerySwapChainSupport(vk::PhysicalDevice, vk::SurfaceKHR);

    VulkanSwapchain(vk::Device logicalDevice, vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, SDL_Window* window);
    ~VulkanSwapchain();

    SwapchainInfo& GetInfo() { return info; }

    bool Ready() const { return ready; }

    void Cleanup();

private:
    bool CreateSwapchain();
    bool CreateImageViews();

    vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

    SwapchainInfo info;

    SDL_Window* window = nullptr;
    vk::SurfaceKHR surface;
    vk::PhysicalDevice physicalDevice;
    vk::Device logicalDevice;

    bool ready = false;
};

}
