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
    RenderSystemCreateInfo ci;
    auto vulkanInstanceRV = VulkanInstance::CreateVulkanInstance();
    if (vulkanInstanceRV.result != GraphicsResult::Ok)
    {
        printf("Failed to create VulkanInstance");
        return vulkanInstanceRV.result;
    }
    ci.vulkanInstance = std::move(vulkanInstanceRV.value);

    auto vulkanDeviceRV = VulkanDevice::CreateVulkanDevice(ci.vulkanInstance->GetInstance());
    if (vulkanDeviceRV.result != GraphicsResult::Ok)
    {
        printf("Failed to create VulkanDevice");
        return vulkanDeviceRV.result;
    }
    ci.vulkanDevice = std::move(vulkanDeviceRV.value);

    auto vulkanSwapchainRV = VulkanSwapchain::CreateVulkanSwapchain({
        ci.vulkanDevice->GetDevice(),
        ci.vulkanDevice->GetPhysicalDevice(),
        ci.vulkanDevice->GetSurface(),
        ci.vulkanDevice->GetWindow()
    });
    if (vulkanSwapchainRV.result != GraphicsResult::Ok)
    {
        printf("Failed to create VulkanSwapchain");
        return vulkanSwapchainRV.result;
    }
    ci.vulkanSwapchain = std::move(vulkanSwapchainRV.value);

    vk::SemaphoreCreateInfo semaphoreInfo = {};

    if (ci.vulkanDevice->GetDevice().createSemaphore(&semaphoreInfo, nullptr, &ci.frameSemaphores.imageAvailableSemaphore) != vk::Result::eSuccess ||
        ci.vulkanDevice->GetDevice().createSemaphore(&semaphoreInfo, nullptr, &ci.frameSemaphores.renderFinishedSemaphore) != vk::Result::eSuccess)
    {
        printf("Failed to create FrameSemaphores!");
        return GraphicsResult::Error;
    }

    auto vulkanRenderPassRV = VulkanRenderPass::CreateRenderPass({ci.vulkanDevice->GetDevice(), ci.vulkanSwapchain->GetInfo().imageFormat});
    if (vulkanRenderPassRV.result != GraphicsResult::Ok)
    {
        printf("Failed to create VulkanRenderPass");
        return vulkanRenderPassRV.result;
    }
    ci.vulkanRenderPass = std::move(vulkanRenderPassRV.value);

    GraphicsResult framebuffersResult = ci.vulkanSwapchain->CreateFramebuffersForRenderPass(ci.vulkanRenderPass->GetRenderPass());
    if (framebuffersResult != GraphicsResult::Ok)
    {
        printf("Failed to create Framebuffers");
        return framebuffersResult;
    }

    auto rs = new RenderSystem(ci);
    if (!rs->ready)
    {
        return GraphicsResult::Error;
    }

    return {GraphicsResult::Ok, std::unique_ptr<RenderSystem>(rs)};
}

RenderSystem::RenderSystem(RenderSystemCreateInfo& ci)
    : vulkanInstance(std::move(ci.vulkanInstance))
    , vulkanDevice(std::move(ci.vulkanDevice))
    , vulkanSwapchain(std::move(ci.vulkanSwapchain))
    , frameSemaphores(std::move(ci.frameSemaphores))
    , vulkanRenderPass(std::move(ci.vulkanRenderPass))
{
    ready = CreateCommandPool()
            && CreateAttrBuffers()
            && createUniformBuffer()
            && createDescriptorPool();

    // todo: move out
    vk::Device logicalDevice = GetDevice();
    vk::PhysicalDevice physicalDevice = GetPhysicalDevice();
    vk::Queue graphicsQueue = GetGraphicsQueue();
    VulkanSwapchainInfo& swapchainInfo = GetSwapchainInfo();

    Mesh testMesh = Ride::GetTestMesh();
    ready = ready && CreateDescriptorSetLayout()
                    && CreateGraphicsPipeline()
                    && uploadMeshAttributes(logicalDevice, physicalDevice, graphicsQueue, testMesh)
                    && createDescriptorSet(logicalDevice)
                    && createCommandBuffers(logicalDevice, swapchainInfo, testMesh);
}

bool RenderSystem::CreateDescriptorSetLayout()
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

    vk::DescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    vk::Result result = vulkanDevice->GetDevice().createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout);
    if (result != vk::Result::eSuccess) {
        printf("Failed to create descriptor set layout!");
        return false;
    }
    return true;
}

bool RenderSystem::CreateGraphicsPipeline()
{
    graphicsPipeline.reset();
    graphicsPipeline = std::make_unique<GraphicsPipeline>(
                GetDevice(), GetSwapchainInfo().extent, vulkanRenderPass->GetRenderPass(), descriptorSetLayout
                );
    if (!graphicsPipeline->Ready())
    {
        printf("Failed to init VulkanPipeline");
        return false;
    }
    return true;
}

bool RenderSystem::CreateCommandPool() {
    Ride::QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(vulkanDevice->GetPhysicalDevice(), vulkanDevice->GetSurface());

    vk::CommandPoolCreateInfo poolInfo = {};
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

    if (vulkanDevice->GetDevice().createCommandPool(&poolInfo, nullptr, &commandPool) != vk::Result::eSuccess) {
        printf("Failed to create graphics command pool!");
        return false;
    }
    return true;
}

bool RenderSystem::CreateAttrBuffers()
{
    VulkanBuffer::createBuffer(GetDevice(), GetPhysicalDevice(), vertexBufferSize,
                               vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                               vk::MemoryPropertyFlagBits::eDeviceLocal,
                               vertexBuffer, vertexBufferMemory);
    VulkanBuffer::createBuffer(GetDevice(), GetPhysicalDevice(), indexBufferSize,
                               vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                               vk::MemoryPropertyFlagBits::eDeviceLocal,
                               indexBuffer, indexBufferMemory);
    return true;
}

bool RenderSystem::createUniformBuffer()
{
    VulkanBuffer::createBuffer(GetDevice(), GetPhysicalDevice(), uniformBufferSize,
                               vk::BufferUsageFlagBits::eUniformBuffer,
                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                               uniformBuffer, uniformBufferMemory);
    return true;
}

bool RenderSystem::createDescriptorPool()
{
    vk::DescriptorPoolSize poolSize = {};
    poolSize.descriptorCount = 1;

    vk::DescriptorPoolCreateInfo poolInfo = {};
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    if (GetDevice().createDescriptorPool(&poolInfo, nullptr, &descriptorPool) != vk::Result::eSuccess) {
        printf("Failed to create descriptor pool!");
        return false;
    }
    return true;
}

// todo: move out

bool RenderSystem::uploadMeshAttributes(vk::Device logicalDevice, vk::PhysicalDevice physicalDevice, vk::Queue graphicsQueue, const Ride::Mesh& mesh)
{
    // vert
    {
    vk::DeviceSize vertexBufferSize = sizeof(mesh.vertices[0]) * mesh.vertices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    VulkanBuffer::createBuffer(logicalDevice, physicalDevice,
                vertexBufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                 stagingBuffer, stagingBufferMemory);

    void* data;

    logicalDevice.mapMemory(stagingBufferMemory, 0, vertexBufferSize, vk::MemoryMapFlags(), &data);
    memcpy(data, mesh.vertices.data(), (size_t) vertexBufferSize);
    logicalDevice.unmapMemory(stagingBufferMemory);

    VulkanBuffer::copyBuffer(logicalDevice, graphicsQueue, commandPool, stagingBuffer, vertexBuffer, vertexBufferSize);

    logicalDevice.destroyBuffer(stagingBuffer);
    logicalDevice.freeMemory(stagingBufferMemory);
    }

    // index
    {
    vk::DeviceSize indexBufferSize = sizeof(mesh.indices[0]) * mesh.indices.size();
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    VulkanBuffer::createBuffer(logicalDevice, physicalDevice,
                 indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                  stagingBuffer, stagingBufferMemory);

    void* data;
    logicalDevice.mapMemory(stagingBufferMemory, 0, indexBufferSize, vk::MemoryMapFlags(), &data);
    memcpy(data, mesh.indices.data(), (size_t) indexBufferSize);
    logicalDevice.unmapMemory(stagingBufferMemory);

    VulkanBuffer::copyBuffer(logicalDevice, graphicsQueue, commandPool, stagingBuffer, indexBuffer, indexBufferSize);

    logicalDevice.destroyBuffer(stagingBuffer);
    logicalDevice.freeMemory(stagingBufferMemory);
    }
    return true;
}

bool RenderSystem::createDescriptorSet(vk::Device logicalDevice) {
    vk::DescriptorSetLayout layouts[] = {descriptorSetLayout};
    vk::DescriptorSetAllocateInfo allocInfo = {};
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;

    if (logicalDevice.allocateDescriptorSets(&allocInfo, &descriptorSet) != vk::Result::eSuccess) {
        printf("failed to allocate descriptor set!");
        return false;
    }

    vk::DescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    vk::WriteDescriptorSet descriptorWrite = {};
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    logicalDevice.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
    return true;
}

bool RenderSystem::createCommandBuffers(vk::Device logicalDevice, Ride::VulkanSwapchainInfo& swapchainInfo, const Ride::Mesh& mesh) {
    commandBuffers.resize(swapchainInfo.framebuffers.size());

    vk::CommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandPool = commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (logicalDevice.allocateCommandBuffers(&allocInfo, commandBuffers.data()) != vk::Result::eSuccess) {
        printf("failed to allocate command buffers!");
        return false;
    }

    std::array<float,4> clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        vk::CommandBufferBeginInfo beginInfo = {};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;

        commandBuffers[i].begin(&beginInfo);

        vk::RenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.renderPass = vulkanRenderPass->GetRenderPass();
        renderPassInfo.framebuffer = swapchainInfo.framebuffers[i];
        renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
        renderPassInfo.renderArea.extent = swapchainInfo.extent;

        vk::ClearValue clearColorVal = vk::ClearColorValue(clearColor);
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColorVal;

        commandBuffers[i].beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

        commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline->GetPipeline());

        vk::Buffer vertexBuffers[] = {vertexBuffer};
        vk::DeviceSize offsets[] = {0};

        commandBuffers[i].bindVertexBuffers(0, 1, vertexBuffers, offsets);
        commandBuffers[i].bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint16);

        commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphicsPipeline->GetLayout(), 0, 1, &descriptorSet, 0, nullptr);

        commandBuffers[i].drawIndexed(static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);

        commandBuffers[i].endRenderPass();

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
    assert(vulkanInstance);
    assert(vulkanDevice);

    vk::Device logicalDevice = vulkanDevice->GetDevice();
    logicalDevice.waitIdle();

    vulkanSwapchain.reset();

    logicalDevice.freeCommandBuffers(commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    graphicsPipeline.reset();
    vulkanRenderPass.reset();
}

void RenderSystem::RecreateTotalPipeline()
{
    CleanupTotalPipeline();
    ready = false;

    // todo: remove duplication with CreateRenderSystem
    auto vulkanSwapchainRV = VulkanSwapchain::CreateVulkanSwapchain({
        vulkanDevice->GetDevice(),
        vulkanDevice->GetPhysicalDevice(),
        vulkanDevice->GetSurface(),
        vulkanDevice->GetWindow()
    });
    if (vulkanSwapchainRV.result != GraphicsResult::Ok)
    {
        printf("Failed to Recreate VulkanSwapchain");
        return;
    }
    vulkanSwapchain = std::move(vulkanSwapchainRV.value);

    auto vulkanRenderPassRV = VulkanRenderPass::CreateRenderPass({vulkanDevice->GetDevice(), vulkanSwapchain->GetInfo().imageFormat});
    if (vulkanRenderPassRV.result != GraphicsResult::Ok)
    {
        printf("Failed to Recreate VulkanRenderPass");
        return;
    }
    vulkanRenderPass = std::move(vulkanRenderPassRV.value);

    GraphicsResult framebuffersResult = vulkanSwapchain->CreateFramebuffersForRenderPass(vulkanRenderPass->GetRenderPass());
    if (framebuffersResult != GraphicsResult::Ok)
    {
        printf("Failed to Recreate Framebuffers");
        return;
    }
    ready = CreateGraphicsPipeline() &&// todo: move to primitive
            createCommandBuffers(GetDevice(), GetSwapchainInfo(), Ride::GetTestMesh());
}

void RenderSystem::UpdateUBO(const UniformBufferObject& ubo)
{
    void* data;
    vk::Device logicalDevice = vulkanDevice->GetDevice();
    logicalDevice.mapMemory(uniformBufferMemory, 0, sizeof(ubo), vk::MemoryMapFlags(), &data);
    memcpy(data, &ubo, sizeof(ubo));
    logicalDevice.unmapMemory(uniformBufferMemory);
}

void RenderSystem::Draw(const std::shared_ptr<Scene>& scene)
{
    vk::Device logicalDevice = vulkanDevice->GetDevice();
    const VulkanSwapchainInfo& swapchainInfo = vulkanSwapchain->GetInfo();
    vk::Queue graphicsQueue = vulkanDevice->GetGraphicsQueue();
    vk::Queue presentQueue = vulkanDevice->GetPresentQueue();

    uint32_t imageIndex;
    vk::Result result = logicalDevice.acquireNextImageKHR(
                swapchainInfo.swapchain,
                std::numeric_limits<uint64_t>::max(),
                frameSemaphores.imageAvailableSemaphore,
                nullptr,
                &imageIndex
                );

    if (result == vk::Result::eErrorOutOfDateKHR) {
        RecreateTotalPipeline();
        return;
    } else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        assert("Failed to acquire swapchain image!" && false);
    }

    vk::SubmitInfo submitInfo = {};

    vk::Semaphore waitSemaphores[] = {frameSemaphores.imageAvailableSemaphore};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    vk::Semaphore signalSemaphores[] = {frameSemaphores.renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (graphicsQueue.submit(1, &submitInfo, nullptr) != vk::Result::eSuccess) {
        assert("Failed to submit draw command buffer!" && false);
    }

    vk::PresentInfoKHR presentInfo = {};

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    vk::SwapchainKHR swapChains[] = {swapchainInfo.swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = presentQueue.presentKHR(&presentInfo);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        RecreateTotalPipeline();
    } else if (result != vk::Result::eSuccess) {
        assert("Failed to present swapchain image!" && false);
    }

    presentQueue.waitIdle();
}

RenderSystem::~RenderSystem()
{
    CleanupTotalPipeline();

    vk::Device logicalDevice = vulkanDevice->GetDevice();
    logicalDevice.waitIdle();

    // todo: move out
    logicalDevice.destroyDescriptorPool(descriptorPool);

    logicalDevice.destroyDescriptorSetLayout(descriptorSetLayout);

    logicalDevice.destroyBuffer(uniformBuffer);
    logicalDevice.freeMemory(uniformBufferMemory);

    logicalDevice.destroyBuffer(indexBuffer);
    logicalDevice.freeMemory(indexBufferMemory);

    logicalDevice.destroyBuffer(vertexBuffer);
    logicalDevice.freeMemory(vertexBufferMemory);
    // end of todo

    logicalDevice.destroySemaphore(frameSemaphores.renderFinishedSemaphore);
    logicalDevice.destroySemaphore(frameSemaphores.imageAvailableSemaphore);

    logicalDevice.destroyCommandPool(commandPool);

    vulkanDevice.reset();
    vulkanInstance.reset();
}

}
