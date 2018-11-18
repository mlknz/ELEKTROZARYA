#pragma once

#include <iostream>
#include <memory>

#include "source/Render/Vulkan/VulkanInstance.hpp"
#include "source/Render/Vulkan/VulkanDevice.hpp"
#include "source/Render/Vulkan/VulkanSwapchain.hpp"
#include "source/Render/Vulkan/GraphicsPipeline.hpp"

#include "source/Scene/Mesh.hpp"

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

    void Draw();

    VkDevice GetDevice() { return vulkanDevice->GetDevice(); }
    VkPhysicalDevice GetPhysicalDevice() { return vulkanDevice->GetPhysicalDevice(); }
    VkSurfaceKHR GetSurface() { return vulkanDevice->GetSurface(); }
    VkQueue GetGraphicsQueue() { return vulkanDevice->GetGraphicsQueue(); }
    SwapchainInfo& GetSwapchainInfo() { return vulkanSwapchain->GetInfo(); }

    uint32_t GetScreenWidth() const { return vulkanSwapchain->GetInfo().extent.width; }
    uint32_t GetScreenHeight() const { return vulkanSwapchain->GetInfo().extent.height; }

    void UpdateUBO(const UniformBufferObject&);

    static std::unique_ptr<RenderSystem> Create();

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

    // todo: move out
    bool createVertexBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, const Ride::Mesh& mesh);
    bool createIndexBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, const Ride::Mesh& mesh);
    bool createUniformBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice);
    bool createDescriptorPool(VkDevice logicalDevice);
    bool createDescriptorSet(VkDevice logicalDevice);
    bool createCommandBuffers(VkDevice logicalDevice, Ride::SwapchainInfo& swapchainInfo, const Ride::Mesh& mesh);

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;

    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;
    // end of todo

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;

    bool ready = false;
};

}
