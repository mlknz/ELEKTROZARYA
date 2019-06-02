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
    auto vulkanInstanceRV = VulkanInstance::CreateVulkanInstance();
    if (vulkanInstanceRV.result != GraphicsResult::Ok)
    {
        printf("Failed to init VulkanInstance");
        return vulkanInstanceRV.result;
    }

    auto vulkanDeviceRV = VulkanDevice::CreateVulkanDevice(vulkanInstanceRV.value->GetInstance());
    if (vulkanDeviceRV.result != GraphicsResult::Ok)
    {
        printf("Failed to init VulkanDevice");
        return vulkanDeviceRV.result;
    }

    auto rs = new RenderSystem(std::move(vulkanInstanceRV.value), std::move(vulkanDeviceRV.value));
    if (!rs->ready)
    {
        return GraphicsResult::Error;
    }

    return {GraphicsResult::Ok, std::unique_ptr<RenderSystem>(rs)};
}

RenderSystem::RenderSystem(std::unique_ptr<VulkanInstance> aInstance, std::unique_ptr<VulkanDevice> aDevice)
    : vulkanInstance(std::move(aInstance)), vulkanDevice(std::move(aDevice))
{
    ready = CreateSwapchain()
            && CreateSemaphores()
            && CreateRenderPass()
            && CreateFramebuffers()
            && CreateCommandPool()
            && CreateAttrBuffers()
            && createUniformBuffer()
            && createDescriptorPool();

    // todo: move out
    vk::Device logicalDevice = GetDevice();
    vk::PhysicalDevice physicalDevice = GetPhysicalDevice();
    vk::Queue graphicsQueue = GetGraphicsQueue();
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
    vk::Device logicalDevice = vulkanDevice->GetDevice();
    vk::SemaphoreCreateInfo semaphoreInfo = {};

    if (logicalDevice.createSemaphore(&semaphoreInfo, nullptr, &imageAvailableSemaphore) != vk::Result::eSuccess ||
        logicalDevice.createSemaphore(&semaphoreInfo, nullptr, &renderFinishedSemaphore) != vk::Result::eSuccess)
    {
        printf("Failed to create semaphores!");
        return false;
    }
    return true;
}

bool RenderSystem::CreateRenderPass()
{
    vk::AttachmentDescription colorAttachment = {};
    colorAttachment.setFormat(vulkanSwapchain->GetInfo().imageFormat)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference colorAttachmentRef = {0, vk::ImageLayout::eColorAttachmentOptimal};

    vk::SubpassDescription subpass = {};
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setColorAttachmentCount(1)
            .setPColorAttachments(&colorAttachmentRef);

    vk::SubpassDependency dependency = { // todo: dont' need dep with one subpass. remove it
        VK_SUBPASS_EXTERNAL, 0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::AccessFlagBits::eColorAttachmentRead, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite
        };

    vk::RenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.setAttachmentCount(1)
            .setPAttachments(&colorAttachment)
            .setSubpassCount(1)
            .setPSubpasses(&subpass)
            .setDependencyCount(1)
            .setPDependencies(&dependency);

    vk::Result result = vulkanDevice->GetDevice().createRenderPass(&renderPassInfo, nullptr, &renderPass);
    if (result != vk::Result::eSuccess) {
        assert(false);
        printf("Failed to create render pass!");
        return false;
    }
    return true;
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
        vk::ImageView attachments[] = {
            swapchainInfo.imageViews[i]
        };

        vk::FramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchainInfo.extent.width;
        framebufferInfo.height = swapchainInfo.extent.height;
        framebufferInfo.layers = 1;

        if (vulkanDevice->GetDevice().createFramebuffer(&framebufferInfo, nullptr, &swapchainInfo.framebuffers[i]) != vk::Result::eSuccess) {
            printf("Failed to create framebuffer!");
            return false;
        }
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

bool RenderSystem::createCommandBuffers(vk::Device logicalDevice, Ride::SwapchainInfo& swapchainInfo, const Ride::Mesh& mesh) {
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
        renderPassInfo.renderPass = renderPass;
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

    if (vulkanSwapchain)
    {
        vulkanSwapchain->Cleanup();
        vulkanSwapchain.reset();
    }

    logicalDevice.freeCommandBuffers(commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    graphicsPipeline.reset();
    logicalDevice.destroyRenderPass(renderPass);
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
    vk::Device logicalDevice = vulkanDevice->GetDevice();
    logicalDevice.mapMemory(uniformBufferMemory, 0, sizeof(ubo), vk::MemoryMapFlags(), &data);
    memcpy(data, &ubo, sizeof(ubo));
    logicalDevice.unmapMemory(uniformBufferMemory);
}

void RenderSystem::Draw(const std::shared_ptr<Scene>& scene)
{
    vk::Device logicalDevice = vulkanDevice->GetDevice();
    const SwapchainInfo& swapchainInfo = vulkanSwapchain->GetInfo();
    vk::Queue graphicsQueue = vulkanDevice->GetGraphicsQueue();
    vk::Queue presentQueue = vulkanDevice->GetPresentQueue();

    uint32_t imageIndex;
    vk::Result result = logicalDevice.acquireNextImageKHR(
                swapchainInfo.swapchain,
                std::numeric_limits<uint64_t>::max(),
                imageAvailableSemaphore,
                nullptr,
                &imageIndex
                );

    if (result == vk::Result::eErrorOutOfDateKHR) {
        RecreateTotalPipeline();
        return;
    } else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        assert("Failed to acquire swap chain image!" && false);
    }

    vk::SubmitInfo submitInfo = {};

    vk::Semaphore waitSemaphores[] = {imageAvailableSemaphore};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    vk::Semaphore signalSemaphores[] = {renderFinishedSemaphore};
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
        assert("Failed to present swap chain image!" && false);
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

    logicalDevice.destroySemaphore(renderFinishedSemaphore);
    logicalDevice.destroySemaphore(imageAvailableSemaphore);

    logicalDevice.destroyCommandPool(commandPool);

    vulkanDevice.reset();
    vulkanInstance.reset();
}

}
