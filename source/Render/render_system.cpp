#include "render_system.hpp"

#include <algorithm>
#include <cassert>
#include "core/file_utils.hpp"
#include "core/view.hpp"
#include "core/scene/scene.hpp"
#include "render/vulkan/utils.hpp"
#include "render/vulkan/vulkan_buffer.hpp"

namespace ez{

struct GlobalUBO
{
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::mat4 viewProjectionMatrix;
};

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

    auto vulkanDeviceMemoryRV = VulkanDeviceMemoryManager::CreateVulkanDeviceMemoryManager(ci.vulkanDevice->GetDevice(), ci.vulkanDevice->GetPhysicalDevice());
    if (vulkanDeviceMemoryRV.result != GraphicsResult::Ok)
    {
        printf("Failed to create VulkanDeviceMemoryManager");
        return vulkanDeviceMemoryRV.result;
    }
    ci.vulkanDeviceMemoryManager = std::move(vulkanDeviceMemoryRV.value);

    auto dsLayoutPtr = CreateDescriptorSetLayout(ci.vulkanDevice->GetDevice());
    if (!dsLayoutPtr.has_value())
    {
        printf("Failed to create default vk::DescriptorSetLayout");
        return GraphicsResult::Error;
    }
    ci.descriptorSetLayout = *dsLayoutPtr;

    ci.graphicsPipeline = std::make_unique<GraphicsPipeline>(
                ci.vulkanDevice->GetDevice(), ci.vulkanSwapchain->GetInfo().extent, ci.vulkanRenderPass->GetRenderPass(), ci.descriptorSetLayout
                );

    if (!ci.graphicsPipeline->Ready())
    {
        printf("Failed to create default GraphicsPipeline");
        return GraphicsResult::Error;
    }

    return {GraphicsResult::Ok, std::make_unique<RenderSystem>(ci)};
}

RenderSystem::RenderSystem(RenderSystemCreateInfo& ci)
    : vulkanInstance(std::move(ci.vulkanInstance))
    , vulkanDevice(std::move(ci.vulkanDevice))
    , vulkanSwapchain(std::move(ci.vulkanSwapchain))
    , frameSemaphores(std::move(ci.frameSemaphores))
    , vulkanRenderPass(std::move(ci.vulkanRenderPass))
    , vulkanDeviceMemoryManager(std::move(ci.vulkanDeviceMemoryManager))
    , descriptorSetLayout(std::move(ci.descriptorSetLayout))
    , graphicsPipeline(std::move(ci.graphicsPipeline))
{}

std::optional<vk::DescriptorSetLayout> RenderSystem::CreateDescriptorSetLayout(vk::Device vkDevice)
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

    vk::DescriptorSetLayout dsLayout;
    vk::Result result = vkDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &dsLayout);
    if (result != vk::Result::eSuccess) {
        printf("Failed to create descriptor set layout!");
        return {};
    }
    return dsLayout;
}

bool RenderSystem::uploadMeshAttributes(vk::Device logicalDevice, vk::PhysicalDevice physicalDevice,
                                        vk::Queue graphicsQueue, vk::CommandPool graphicsCommandPool, const ez::Mesh& mesh)
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
    memcpy(data, mesh.vertices.data(), static_cast<size_t>(vertexBufferSize));
    logicalDevice.unmapMemory(stagingBufferMemory);

    VulkanBuffer::copyBuffer(logicalDevice, graphicsQueue, graphicsCommandPool, stagingBuffer, vulkanDeviceMemoryManager->GetVertexBuffer(), vertexBufferSize);

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
    memcpy(data, mesh.indices.data(), static_cast<size_t>(indexBufferSize));
    logicalDevice.unmapMemory(stagingBufferMemory);

    VulkanBuffer::copyBuffer(logicalDevice, graphicsQueue, graphicsCommandPool, stagingBuffer, vulkanDeviceMemoryManager->GetIndexBuffer(), indexBufferSize);

    logicalDevice.destroyBuffer(stagingBuffer);
    logicalDevice.freeMemory(stagingBufferMemory);
    }
    return true;
}

bool RenderSystem::createDescriptorSet(vk::Device logicalDevice, vk::DescriptorPool descriptorPool) {
    vk::DescriptorSetLayout layouts[] = {descriptorSetLayout};
    vk::DescriptorSetAllocateInfo allocInfo = {};
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;

    if (logicalDevice.allocateDescriptorSets(&allocInfo, &descriptorSet) != vk::Result::eSuccess) {
        printf("Failed to allocate descriptor set!");
        return false;
    }

    vk::DescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = vulkanDeviceMemoryManager->GetUniformBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(GlobalUBO);

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

bool RenderSystem::createCommandBuffers(vk::Device logicalDevice, vk::CommandPool graphicsCommandPool, ez::VulkanSwapchainInfo& swapchainInfo, const ez::Mesh& mesh) {
    commandBuffers.resize(swapchainInfo.framebuffers.size());

    vk::CommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandPool = graphicsCommandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

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

        vk::Buffer vertexBuffers[] = {vulkanDeviceMemoryManager->GetVertexBuffer()};
        vk::DeviceSize offsets[] = {0};

        commandBuffers[i].bindVertexBuffers(0, 1, vertexBuffers, offsets);
        commandBuffers[i].bindIndexBuffer(vulkanDeviceMemoryManager->GetIndexBuffer(), 0, vk::IndexType::eUint16);

        commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphicsPipeline->GetLayout(), 0, 1, &descriptorSet, 0, nullptr);

        commandBuffers[i].drawIndexed(static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);

        commandBuffers[i].endRenderPass();

        if (commandBuffers[i].end() != vk::Result::eSuccess) {
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

    logicalDevice.freeCommandBuffers(vulkanDevice->GetGraphicsCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    graphicsPipeline.reset();
    vulkanRenderPass.reset();
}

void RenderSystem::RecreateTotalPipeline()
{
    CleanupTotalPipeline();

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
    graphicsPipeline.reset();
    graphicsPipeline = std::make_unique<GraphicsPipeline>(
                GetDevice(), GetSwapchainInfo().extent, vulkanRenderPass->GetRenderPass(), descriptorSetLayout
                );
    if (!graphicsPipeline->Ready())
    {
        printf("Failed to recreate GraphicsPipeline");
        assert(false); // todo: fallback
    }
    // todo: move to primitive
    createCommandBuffers(GetDevice(), vulkanDevice->GetGraphicsCommandPool(), GetSwapchainInfo(), ez::GetTestMesh());
}



void RenderSystem::UpdateGlobalUniforms(const std::unique_ptr<Camera>& camera)
{
    vk::Device logicalDevice = vulkanDevice->GetDevice();

    void* data;
    GlobalUBO ubo = {
        camera->GetViewMatrix(),
        camera->GetProjectionMatrix(),
        camera->GetViewProjectionMatrix()
    };

    logicalDevice.mapMemory(vulkanDeviceMemoryManager->GetUniformBufferMemory(), 0, sizeof(ubo), vk::MemoryMapFlags(), &data);
    memcpy(data, &ubo, sizeof(ubo));
    logicalDevice.unmapMemory(vulkanDeviceMemoryManager->GetUniformBufferMemory());
}

void RenderSystem::PrepareToRender(std::shared_ptr<Scene> scene)
{
    // todo: clean prev state
    // todo: move out
    auto testMesh = GetTestMesh();
    uploadMeshAttributes(GetDevice(), GetPhysicalDevice(), GetGraphicsQueue(), vulkanDevice->GetGraphicsCommandPool(), testMesh)
    && createDescriptorSet(GetDevice(), vulkanDevice->GetDescriptorPool())
    && createCommandBuffers(GetDevice(), vulkanDevice->GetGraphicsCommandPool(), GetSwapchainInfo(), testMesh);
    scene->SetReadyToRender(true);
}

void RenderSystem::Draw(const std::unique_ptr<View>& view, const std::unique_ptr<Camera>& camera)
{
    UpdateGlobalUniforms(camera);

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

    vk::SwapchainKHR swapchains[] = {swapchainInfo.swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;

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
    logicalDevice.destroyDescriptorSetLayout(descriptorSetLayout);

    // end of todo

    logicalDevice.destroySemaphore(frameSemaphores.renderFinishedSemaphore);
    logicalDevice.destroySemaphore(frameSemaphores.imageAvailableSemaphore);

    vulkanDeviceMemoryManager.reset();
    vulkanDevice.reset();
    vulkanInstance.reset();
}

}
