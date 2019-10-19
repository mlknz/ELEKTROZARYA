#pragma once

#include <iostream>
#include <memory>

#include <Vulkan/VulkanInstance.hpp>
#include <Vulkan/VulkanDevice.hpp>
#include <Vulkan/VulkanSwapchain.hpp>
#include <Vulkan/VulkanRenderPass.hpp>
#include <Vulkan/VulkanDeviceMemoryManager.hpp>
#include <Vulkan/GraphicsPipeline.hpp>
#include <GraphicsResult.hpp>

#include "Scene/Mesh.hpp"

namespace Ride {
    class Scene;
}

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

namespace Ride {

struct FrameSemaphores
{
    vk::Semaphore imageAvailableSemaphore;
    vk::Semaphore renderFinishedSemaphore;
};

struct RenderSystemCreateInfo
{
    std::unique_ptr<VulkanInstance> vulkanInstance;
    std::unique_ptr<VulkanDevice> vulkanDevice;
    std::unique_ptr<VulkanSwapchain> vulkanSwapchain;
    FrameSemaphores frameSemaphores;
    std::unique_ptr<VulkanRenderPass> vulkanRenderPass;
    std::unique_ptr<VulkanDeviceMemoryManager> vulkanDeviceMemoryManager;
};

class RenderSystem
{
public:
    RenderSystem() = delete;
    RenderSystem(RenderSystemCreateInfo& ci);
    ~RenderSystem();

    void Draw(const std::shared_ptr<Scene>& scene);

    vk::Device GetDevice() { return vulkanDevice->GetDevice(); }
    vk::PhysicalDevice GetPhysicalDevice() { return vulkanDevice->GetPhysicalDevice(); }
    vk::SurfaceKHR GetSurface() { return vulkanDevice->GetSurface(); }
    vk::Queue GetGraphicsQueue() { return vulkanDevice->GetGraphicsQueue(); }
    VulkanSwapchainInfo& GetSwapchainInfo() { return vulkanSwapchain->GetInfo(); }

    uint32_t GetScreenWidth() const { return vulkanSwapchain->GetInfo().extent.width; }
    uint32_t GetScreenHeight() const { return vulkanSwapchain->GetInfo().extent.height; }

    void UpdateUBO(const UniformBufferObject&);

    static ResultValue<std::unique_ptr<RenderSystem>> Create();

private:
    void CleanupTotalPipeline();
    void RecreateTotalPipeline();

    bool CreateDescriptorSetLayout();
    bool CreateGraphicsPipeline();

    std::unique_ptr<VulkanInstance> vulkanInstance = nullptr;
    std::unique_ptr<VulkanDevice> vulkanDevice = nullptr;
    std::unique_ptr<VulkanSwapchain> vulkanSwapchain = nullptr;
    FrameSemaphores frameSemaphores;
    std::unique_ptr<VulkanRenderPass> vulkanRenderPass = nullptr;
    std::unique_ptr<VulkanDeviceMemoryManager> vulkanDeviceMemoryManager = nullptr;

    // todo: move out
    vk::DescriptorSetLayout descriptorSetLayout;

    std::unique_ptr<GraphicsPipeline> graphicsPipeline = nullptr;

    std::vector<vk::CommandBuffer> commandBuffers;

    bool uploadMeshAttributes(vk::Device logicalDevice, vk::PhysicalDevice physicalDevice, vk::Queue graphicsQueue, vk::CommandPool graphicsCommandPool, const Ride::Mesh& mesh);
    bool createDescriptorSet(vk::Device logicalDevice, vk::DescriptorPool descriptorPool);
    bool createCommandBuffers(vk::Device logicalDevice, vk::CommandPool graphicsCommandPool, Ride::VulkanSwapchainInfo& swapchainInfo, const Ride::Mesh& mesh);

    vk::DescriptorSet descriptorSet;
    // end of todo

    bool ready = false;
};

}
