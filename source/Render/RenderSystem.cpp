#include "RenderSystem.hpp"
#include "source/Render/Vulkan/Utils.hpp"
#include "source/FileUtils.hpp"
#include "source/Render/Vulkan/VulkanBuffer.hpp"
#include "source/Scene/Scene.hpp"

#include <algorithm>
#include <cassert>

namespace Ride{

ResultValue<std::unique_ptr<RenderSystem>> RenderSystem::Create()
{
    auto rs = new RenderSystem();
    if (!rs->ready)
    {
        return GraphicsResult::Error;
    }
    return {GraphicsResult::Ok, std::unique_ptr<RenderSystem>(rs)};
}

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

    ready = CreateSwapchain()
            && CreateSemaphores()
            && CreateRenderPass()
            && CreateFramebuffers()
            && CreateCommandPool()
            && CreateAttrBuffers()
            && createUniformBuffer()
            && createDescriptorPool();

    // todo: move out
    VkDevice logicalDevice = GetDevice();
    VkPhysicalDevice physicalDevice = GetPhysicalDevice();
    VkQueue graphicsQueue = GetGraphicsQueue();
    SwapchainInfo& swapchainInfo = GetSwapchainInfo();

    Mesh testMesh = Ride::GetTestMesh();
    ready = ready && CreateDescriptorSetLayout()
                    && CreateGraphicsPipeline()
                    && uploadMeshAttributes(logicalDevice, physicalDevice, graphicsQueue, testMesh)
                    && createDescriptorSet(logicalDevice)
                    && createCommandBuffers(logicalDevice, swapchainInfo, testMesh);
}

bool RenderSystem::CreateSwapchain()
{
    vulkanSwapchain.reset();
    vulkanSwapchain = std::make_unique<VulkanSwapchain>(
                vulkanDevice->GetDevice(),
                vulkanDevice->GetPhysicalDevice(),
                vulkanDevice->GetSurface(),
                vulkanDevice->GetWindow()
                );
    if (!vulkanSwapchain->Ready())
    {
        assert(false && "Failed to init VulkanSwapchain");
        return false;
    }
    return true;
}

bool RenderSystem::CreateSemaphores() {
    VkDevice logicalDevice = vulkanDevice->GetDevice();
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {

        printf("Failed to create semaphores!");
        return false;
    }
    return true;
}

bool RenderSystem::CreateRenderPass()
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = vulkanSwapchain->GetInfo().imageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(vulkanDevice->GetDevice(), &renderPassInfo, nullptr, &renderPass);
    if (result != VK_SUCCESS) {
        assert(false);
        printf("Failed to create render pass!");
        return false;
    }
    return true;
}

bool RenderSystem::CreateDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    VkResult result = vkCreateDescriptorSetLayout(vulkanDevice->GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout);
    if (result != VK_SUCCESS) {
        printf("failed to create descriptor set layout!");
        return false;
    }
    return true;
}

bool RenderSystem::CreateGraphicsPipeline()
{
    graphicsPipeline.reset();
    graphicsPipeline = std::make_unique<GraphicsPipeline>(
                GetDevice(), GetSwapchainInfo().extent, renderPass, descriptorSetLayout
                );
    if (!graphicsPipeline->Ready())
    {
        printf("Failed to init VulkanPipeline");
        return false;
    }
    return true;
}

bool RenderSystem::CreateFramebuffers() {
    SwapchainInfo& swapchainInfo = vulkanSwapchain->GetInfo();
    swapchainInfo.framebuffers.resize(swapchainInfo.imageViews.size());

    for (size_t i = 0; i < swapchainInfo.imageViews.size(); i++) {
        VkImageView attachments[] = {
            swapchainInfo.imageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchainInfo.extent.width;
        framebufferInfo.height = swapchainInfo.extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vulkanDevice->GetDevice(), &framebufferInfo, nullptr, &swapchainInfo.framebuffers[i]) != VK_SUCCESS) {
            printf("Failed to create framebuffer!");
            return false;
        }
    }
    return true;
}

bool RenderSystem::CreateCommandPool() {
    Ride::QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(vulkanDevice->GetPhysicalDevice(), vulkanDevice->GetSurface());

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

    if (vkCreateCommandPool(vulkanDevice->GetDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        printf("Failed to create graphics command pool!");
        return false;
    }
    return true;
}

bool RenderSystem::CreateAttrBuffers()
{
    VkDevice logicalDevice = GetDevice();
    VkPhysicalDevice physicalDevice = GetPhysicalDevice();

    VulkanBuffer::createBuffer(logicalDevice, physicalDevice, vertexBufferSize,
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               vertexBuffer, vertexBufferMemory);
    VulkanBuffer::createBuffer(logicalDevice, physicalDevice, indexBufferSize,
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               indexBuffer, indexBufferMemory);
    return true;
}

bool RenderSystem::createUniformBuffer()
{
    VulkanBuffer::createBuffer(GetDevice(), GetPhysicalDevice(), uniformBufferSize,
                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                               uniformBuffer, uniformBufferMemory);
    return true;
}

bool RenderSystem::createDescriptorPool()
{
    VkDevice logicalDevice = GetDevice();

    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        printf("Failed to create descriptor pool!");
        return false;
    }
    return true;
}

// todo: move out

bool RenderSystem::uploadMeshAttributes(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, const Ride::Mesh& mesh)
{
    // vert
    {
    VkDeviceSize vertexBufferSize = sizeof(mesh.vertices[0]) * mesh.vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VulkanBuffer::createBuffer(logicalDevice, physicalDevice,
                vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(logicalDevice, stagingBufferMemory, 0, vertexBufferSize, 0, &data);
        memcpy(data, mesh.vertices.data(), (size_t) vertexBufferSize);
    vkUnmapMemory(logicalDevice, stagingBufferMemory);

    VulkanBuffer::copyBuffer(logicalDevice, graphicsQueue, commandPool, stagingBuffer, vertexBuffer, vertexBufferSize);

    vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
    }

    // index
    {
    VkDeviceSize indexBufferSize = sizeof(mesh.indices[0]) * mesh.indices.size();
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VulkanBuffer::createBuffer(logicalDevice, physicalDevice,
                 indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(logicalDevice, stagingBufferMemory, 0, indexBufferSize, 0, &data);
        memcpy(data, mesh.indices.data(), (size_t) indexBufferSize);
    vkUnmapMemory(logicalDevice, stagingBufferMemory);

    VulkanBuffer::copyBuffer(logicalDevice, graphicsQueue, commandPool, stagingBuffer, indexBuffer, indexBufferSize);

    vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
    }
    return true;
}

bool RenderSystem::createDescriptorSet(VkDevice logicalDevice) {
    VkDescriptorSetLayout layouts[] = {descriptorSetLayout};
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;

    if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        printf("failed to allocate descriptor set!");
        return false;
    }

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, nullptr);
    return true;
}

bool RenderSystem::createCommandBuffers(VkDevice logicalDevice, Ride::SwapchainInfo& swapchainInfo, const Ride::Mesh& mesh) {
    commandBuffers.resize(swapchainInfo.framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        printf("failed to allocate command buffers!");
        return false;
    }

    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapchainInfo.framebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapchainInfo.extent;

        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->GetPipeline());

        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->GetLayout(), 0, 1, &descriptorSet, 0, nullptr);

        vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            printf("failed to record command buffer!");
            return false;
        }
    }
    return true;
}
// end of todo

void RenderSystem::CleanupTotalPipeline()
{
    assert(vulkanInstance && vulkanInstance->Ready());
    assert(vulkanDevice && vulkanDevice->Ready());

    VkDevice logicalDevice = vulkanDevice->GetDevice();
    vkDeviceWaitIdle(logicalDevice);

    if (vulkanSwapchain)
    {
        vulkanSwapchain->Cleanup();
        vulkanSwapchain.reset();
    }

    vkFreeCommandBuffers(logicalDevice, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    graphicsPipeline.reset();
    vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
}

void RenderSystem::RecreateTotalPipeline()
{
    CleanupTotalPipeline();

    ready = CreateSwapchain()
            && CreateRenderPass()
            && CreateGraphicsPipeline() // todo: move to primitive
            && CreateFramebuffers()
            && createCommandBuffers(GetDevice(), GetSwapchainInfo(), Ride::GetTestMesh());
}

void RenderSystem::UpdateUBO(const UniformBufferObject& ubo)
{
    void* data;
    VkDevice logicalDevice = vulkanDevice->GetDevice();
    vkMapMemory(logicalDevice, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(logicalDevice, uniformBufferMemory);
}

void RenderSystem::Draw(const std::shared_ptr<Scene>& scene)
{
    VkDevice logicalDevice = vulkanDevice->GetDevice();
    const SwapchainInfo& swapchainInfo = vulkanSwapchain->GetInfo();
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
        RecreateTotalPipeline();
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
        RecreateTotalPipeline();
    } else if (result != VK_SUCCESS) {
        assert("failed to present swap chain image!" && false);
    }

    vkQueueWaitIdle(presentQueue);
}

RenderSystem::~RenderSystem()
{
    CleanupTotalPipeline();

    VkDevice logicalDevice = vulkanDevice->GetDevice();
    vkDeviceWaitIdle(logicalDevice);

    // todo: move out
    vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);

    vkDestroyBuffer(logicalDevice, uniformBuffer, nullptr);
    vkFreeMemory(logicalDevice, uniformBufferMemory, nullptr);

    vkDestroyBuffer(logicalDevice, indexBuffer, nullptr);
    vkFreeMemory(logicalDevice, indexBufferMemory, nullptr);

    vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);
    vkFreeMemory(logicalDevice, vertexBufferMemory, nullptr);
    // end of todo

    vkDestroySemaphore(logicalDevice, renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(logicalDevice, imageAvailableSemaphore, nullptr);

    vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

    vulkanDevice.reset();
    vulkanInstance.reset();
}

}
