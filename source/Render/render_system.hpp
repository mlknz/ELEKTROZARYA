#pragma once

#include <iostream>
#include <memory>
#include <core/scene/mesh.hpp>
#include <graphics_result.hpp>
#include <vulkan/vulkan_instance.hpp>
#include <vulkan/vulkan_device.hpp>
#include <vulkan/vulkan_swapchain.hpp>
#include <vulkan/vulkan_render_pass.hpp>
#include <vulkan/vulkan_device_memory_manager.hpp>
#include <vulkan/vulkan_graphics_pipeline.hpp>

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

namespace Ride {

class View;
class Camera;

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

    void Draw(const std::unique_ptr<View>& view, const std::unique_ptr<Camera>& camera);

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
