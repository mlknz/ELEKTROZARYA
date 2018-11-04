#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>

namespace Ride
{

struct SwapchainInfo
{
    VkSwapchainKHR swapchain;
    std::vector<VkImage> images;
    VkFormat imageFormat;
    VkExtent2D extent;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanSwapchain
{
public:
    static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice, VkSurfaceKHR);

    VulkanSwapchain(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, SDL_Window* window);
    ~VulkanSwapchain();

    SwapchainInfo& GetInfo() { return info; }

    bool Ready() const { return ready; }

    void Cleanup();

private:
    bool CreateSwapchain();
    bool CreateImageViews();

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    SwapchainInfo info;

    SDL_Window* window = nullptr;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice = VK_NULL_HANDLE;

    bool ready = false;
};

}
