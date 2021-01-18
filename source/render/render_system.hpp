#pragma once

#include <iostream>
#include <memory>
#include <optional>
#include "core/scene/mesh.hpp"
#include "core/scene/scene.hpp"
#include "core/camera/camera.hpp"
#include "render/graphics_result.hpp"
#include "render/vulkan/vulkan_instance.hpp"
#include "render/vulkan/vulkan_device.hpp"
#include "render/vulkan/vulkan_swapchain.hpp"
#include "render/vulkan/vulkan_render_pass.hpp"
#include "render/vulkan/vulkan_graphics_pipeline.hpp"

namespace ez {

class View;
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
    std::unique_ptr<VulkanRenderPass> vulkanRenderPass;

    FrameSemaphores frameSemaphores;
    vk::DescriptorSetLayout descriptorSetLayout;
    std::unique_ptr<GraphicsPipeline> graphicsPipeline;
    std::vector<vk::CommandBuffer> commandBuffers;
};

class RenderSystem
{
public:
    RenderSystem() = delete;
    RenderSystem(RenderSystemCreateInfo& ci);
    ~RenderSystem();

    static ResultValue<std::unique_ptr<RenderSystem>> Create();

    void PrepareToRender(std::shared_ptr<Scene> scene);
    void Draw(const std::unique_ptr<View>& view, const std::unique_ptr<Camera>& camera); // todo: mewmew move Camera to View

    vk::Device GetDevice() { return vulkanDevice->GetDevice(); }
    vk::PhysicalDevice GetPhysicalDevice() { return vulkanDevice->GetPhysicalDevice(); }
    SDL_Window* GetWindow() { return vulkanDevice->GetWindow(); }
    vk::SurfaceKHR GetSurface() { return vulkanDevice->GetSurface(); }
    vk::Queue GetGraphicsQueue() { return vulkanDevice->GetGraphicsQueue(); }
    VulkanSwapchainInfo& GetSwapchainInfo() { return vulkanSwapchain->GetInfo(); }

    const vk::Extent2D& GetViewportExtent() const { return vulkanSwapchain->GetInfo().extent; }

private:
    static std::optional<vk::DescriptorSetLayout> CreateDescriptorSetLayout(vk::Device vkDevice);
    static std::vector<vk::CommandBuffer> CreateCommandBuffers(vk::Device logicalDevice, vk::CommandPool graphicsCommandPool, ez::VulkanSwapchainInfo& swapchainInfo);
    static bool InitializeImGui(const RenderSystemCreateInfo& ci);

    void UpdateGlobalUniforms(std::shared_ptr<Scene> scene, const std::unique_ptr<Camera>& camera);

    void CleanupTotalPipeline();
    void RecreateTotalPipeline();

    std::unique_ptr<VulkanInstance> vulkanInstance = nullptr;
    std::unique_ptr<VulkanDevice> vulkanDevice = nullptr;
    std::unique_ptr<VulkanSwapchain> vulkanSwapchain = nullptr;
    std::unique_ptr<VulkanRenderPass> vulkanRenderPass = nullptr;
    std::unique_ptr<GraphicsPipeline> graphicsPipeline = nullptr;

    vk::DescriptorSetLayout descriptorSetLayout;
    FrameSemaphores frameSemaphores;

    std::vector<vk::CommandBuffer> commandBuffers;
    size_t curFrameIndex = 0;
};

}
