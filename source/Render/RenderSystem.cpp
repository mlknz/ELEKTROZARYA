#include "RenderSystem.hpp"

#include <algorithm>
#include <cassert>

using namespace Ride;

RenderSystem::RenderSystem()
{
    vulkanInstance = std::make_unique<VulkanInstance>();
    ready = vulkanInstance->Ready();
    if (!ready)
    {
        printf("Failed to init VulkanInstance");
        return;
    }
    vulkanDevice = std::make_unique<VulkanDevice>(vulkanInstance->GetInstance());
    ready = vulkanDevice->Ready();
    if (!ready)
    {
        printf("Failed to init VulkanDevice");
        return;
    }
    vulkanSwapchain = std::make_unique<VulkanSwapchain>(
                vulkanDevice->GetDevice(),
                vulkanDevice->GetPhysicalDevice(),
                vulkanDevice->GetSurface(),
                vulkanDevice->GetWindow()
                );
    ready = vulkanSwapchain->Ready();
    if (!ready)
    {
        printf("Failed to init VulkanSwapchain");
        return;
    }
    ready = CreateSemaphores();
}

bool RenderSystem::CreateSemaphores() {
    VkDevice logicalDevice = vulkanDevice->GetDevice();
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {

        printf("failed to create semaphores!");
        return false;
    }
    return true;
}

void RenderSystem::RecreateSwapChain()
{
    VkDevice logicalDevice = vulkanDevice->GetDevice();
    vkDeviceWaitIdle(logicalDevice);

    vulkanSwapchain->Cleanup();
    vulkanSwapchain.reset();

    assert(vulkanInstance && vulkanInstance->Ready());
    assert(vulkanDevice && vulkanDevice->Ready());

    vulkanSwapchain = std::make_unique<VulkanSwapchain>(
                vulkanDevice->GetDevice(),
                vulkanDevice->GetPhysicalDevice(),
                vulkanDevice->GetSurface(),
                vulkanDevice->GetWindow()
                );
    ready = vulkanSwapchain->Ready();

    // todo:
    /*
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandBuffers();
    */
}

void RenderSystem::Draw(std::vector<VkCommandBuffer>& commandBuffers)
{
    VkDevice logicalDevice = vulkanDevice->GetDevice();
    const auto& swapchainInfo = vulkanSwapchain->GetInfo();
    VkQueue graphicsQueue = vulkanDevice->GetGraphicsQueue();
    VkQueue presentQueue = vulkanDevice->GetPresentQueue();

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
                logicalDevice,
                swapchainInfo.swapchain,
                std::numeric_limits<uint64_t>::max(),
                imageAvailableSemaphore,
                VK_NULL_HANDLE,
                &imageIndex
                );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        assert("failed to acquire swap chain image!" && false);
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        assert("failed to submit draw command buffer!" && false);
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapchainInfo.swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        RecreateSwapChain();
    } else if (result != VK_SUCCESS) {
        assert("failed to present swap chain image!" && false);
    }

    vkQueueWaitIdle(presentQueue);
}

RenderSystem::~RenderSystem()
{
    if (vulkanSwapchain)
    {
        vulkanSwapchain->Cleanup();
        vulkanSwapchain.reset();
    }
    VkDevice logicalDevice = vulkanDevice->GetDevice();
    vkDeviceWaitIdle(logicalDevice);
    //

    vkDestroySemaphore(logicalDevice, renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(logicalDevice, imageAvailableSemaphore, nullptr);

    //

    vulkanDevice.reset();
    vulkanInstance.reset();
}
