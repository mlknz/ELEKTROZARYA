#pragma once

#include <iostream>
#include <memory>

#include "source/Render/Vulkan/VulkanInstance.hpp"
#include "source/Render/Vulkan/VulkanDevice.hpp"
#include "source/Render/Vulkan/VulkanSwapchain.hpp"

namespace Ride {

class RenderSystem
{
public:
    RenderSystem();
    ~RenderSystem();

    bool Ready() const { return ready; }

    bool CreateSemaphores();


    void Draw(std::vector<VkCommandBuffer>& commandBuffers);

    VkDevice GetDevice() { return vulkanDevice->GetDevice(); }
    VkPhysicalDevice GetPhysicalDevice() { return vulkanDevice->GetPhysicalDevice(); }
    VkSurfaceKHR GetSurface() { return vulkanDevice->GetSurface(); }
    VkQueue GetGraphicsQueue() { return vulkanDevice->GetGraphicsQueue(); }

    SwapchainInfo& GetSwapchainInfo() { return vulkanSwapchain->GetInfo(); }

    void RecreateSwapChain();

private:
    std::unique_ptr<VulkanInstance> vulkanInstance = nullptr;
    std::unique_ptr<VulkanDevice> vulkanDevice = nullptr;
    std::unique_ptr<VulkanSwapchain> vulkanSwapchain = nullptr;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;

    bool ready = false;
};

}
