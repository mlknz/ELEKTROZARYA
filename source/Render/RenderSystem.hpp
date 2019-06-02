#pragma once

#include <iostream>
#include <memory>

#include <Vulkan/VulkanInstance.hpp>
#include <Vulkan/VulkanDevice.hpp>
#include <Vulkan/VulkanSwapchain.hpp>
#include <Vulkan/GraphicsPipeline.hpp>
#include <GraphicsResult.hpp>

#include "source/Scene/Mesh.hpp"

namespace Ride {
    class Scene;
}

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

namespace Ride {

class RenderSystem
{
public:
    ~RenderSystem();

    void Draw(const std::shared_ptr<Scene>& scene);

    VkDevice GetDevice() { return vulkanDevice->GetDevice(); }
    VkPhysicalDevice GetPhysicalDevice() { return vulkanDevice->GetPhysicalDevice(); }
    VkSurfaceKHR GetSurface() { return vulkanDevice->GetSurface(); }
    VkQueue GetGraphicsQueue() { return vulkanDevice->GetGraphicsQueue(); }
    SwapchainInfo& GetSwapchainInfo() { return vulkanSwapchain->GetInfo(); }

    uint32_t GetScreenWidth() const { return vulkanSwapchain->GetInfo().extent.width; }
    uint32_t GetScreenHeight() const { return vulkanSwapchain->GetInfo().extent.height; }

    void UpdateUBO(const UniformBufferObject&);

    static ResultValue<std::unique_ptr<RenderSystem>> Create();

private:
    RenderSystem();

    void CleanupTotalPipeline();
    void RecreateTotalPipeline();

    bool CreateSwapchain();
    bool CreateSemaphores();
    bool CreateRenderPass();
    bool CreateDescriptorSetLayout();
    bool CreateGraphicsPipeline();
    bool CreateFramebuffers();
    bool CreateCommandPool();

    std::unique_ptr<VulkanInstance> vulkanInstance = nullptr;
    std::unique_ptr<VulkanDevice> vulkanDevice = nullptr;
    std::unique_ptr<VulkanSwapchain> vulkanSwapchain = nullptr;

    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;

    std::unique_ptr<GraphicsPipeline> graphicsPipeline = nullptr;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    bool CreateAttrBuffers();
    bool createUniformBuffer();
    bool createDescriptorPool();

    // todo: move out
    bool uploadMeshAttributes(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, const Ride::Mesh& mesh);
    bool createDescriptorSet(VkDevice logicalDevice);
    bool createCommandBuffers(VkDevice logicalDevice, Ride::SwapchainInfo& swapchainInfo, const Ride::Mesh& mesh);

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    uint32_t vertexBufferSize = 1000;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    uint32_t indexBufferSize = 200;

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    uint32_t uniformBufferSize = 200;

    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;
    // end of todo

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;

    bool ready = false;
};

}
