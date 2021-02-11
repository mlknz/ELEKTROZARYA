#pragma once

#include <iostream>
#include <memory>
#include <optional>

#include "core/camera/camera.hpp"
#include "core/scene/scene.hpp"
#include "render/graphics_result.hpp"
#include "render/highlevel/mesh.hpp"
#include "render/vulkan/vulkan_device.hpp"
#include "render/vulkan/vulkan_instance.hpp"
#include "render/vulkan/vulkan_pipeline_manager.hpp"
#include "render/vulkan/vulkan_render_pass.hpp"
#include "render/vulkan/vulkan_swapchain.hpp"

namespace ez
{
class View;
struct FrameSemaphores
{
    vk::Semaphore imageAvailableSemaphore;
    vk::Semaphore renderFinishedSemaphore;
};

struct GlobalUBO final
{
    struct Data final
    {
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;
        glm::mat4 viewProjectionMatrix;
    } data;

    vk::Buffer uniformBuffer;

    vk::DeviceMemory uniformBufferMemory;

    vk::DescriptorSetLayout descriptorSetLayout;
    vk::DescriptorSet descriptorSet;
};

struct RenderSystemCreateInfo
{
    std::unique_ptr<VulkanInstance> vulkanInstance;
    std::unique_ptr<VulkanDevice> vulkanDevice;
    std::unique_ptr<VulkanSwapchain> vulkanSwapchain;
    std::unique_ptr<VulkanRenderPass> vulkanRenderPass;
    std::unique_ptr<VulkanPipelineManager> vulkanPipelineManager;

    FrameSemaphores frameSemaphores;
    GlobalUBO globalUBO;
    vk::DescriptorSetLayout samplersDescriptorSetLayout;
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
    void Draw(const std::unique_ptr<View>& view, const std::unique_ptr<Camera>& camera);

    vk::Device GetDevice() { return vulkanDevice->GetDevice(); }
    vk::PhysicalDevice GetPhysicalDevice() { return vulkanDevice->GetPhysicalDevice(); }
    SDL_Window* GetWindow() { return vulkanDevice->GetWindow(); }
    vk::SurfaceKHR GetSurface() { return vulkanDevice->GetSurface(); }
    vk::Queue GetGraphicsQueue() { return vulkanDevice->GetGraphicsQueue(); }
    VulkanSwapchainInfo& GetSwapchainInfo() { return vulkanSwapchain->GetInfo(); }

    const vk::Extent2D& GetViewportExtent() const { return vulkanSwapchain->GetInfo().extent; }

   private:
    static std::optional<GlobalUBO> CreateGlobalUBO(vk::Device vkDevice,
                                                    vk::PhysicalDevice physicalDevice,
                                                    vk::DescriptorPool descriptorPool);
    static std::vector<vk::CommandBuffer> CreateCommandBuffers(
        vk::Device logicalDevice,
        vk::CommandPool graphicsCommandPool,
        ez::VulkanSwapchainInfo& swapchainInfo);
    static bool InitializeImGui(const RenderSystemCreateInfo& ci);

    void UpdateGlobalUniforms(const std::unique_ptr<Camera>& camera);

    void CleanupTotalPipeline();
    void RecreateTotalPipeline();

    std::unique_ptr<VulkanInstance> vulkanInstance = nullptr;
    std::unique_ptr<VulkanDevice> vulkanDevice = nullptr;
    std::unique_ptr<VulkanSwapchain> vulkanSwapchain = nullptr;
    std::unique_ptr<VulkanRenderPass> vulkanRenderPass = nullptr;
    std::unique_ptr<VulkanPipelineManager> vulkanPipelineManager = nullptr;

    GlobalUBO globalUBO;
    vk::DescriptorSetLayout samplersDescriptorSetLayout;
    FrameSemaphores frameSemaphores;

    std::vector<vk::CommandBuffer> commandBuffers;
    size_t curFrameIndex = 0;

    bool needRecreateResources = false;
};

}  // namespace ez
