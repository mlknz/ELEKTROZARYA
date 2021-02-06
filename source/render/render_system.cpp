#include "render_system.hpp"

#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <algorithm>
#include <cassert>

#include "core/file_utils.hpp"
#include "core/log_assert.hpp"
#include "core/scene/scene.hpp"
#include "core/view.hpp"
#include "render/graphics_result.hpp"
#include "render/vulkan/utils.hpp"
#include "render/vulkan/vulkan_buffer.hpp"

namespace ez
{
struct GlobalUBO
{
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::mat4 viewProjectionMatrix;
};

static void check_vk_result_imgui(VkResult err)
{
    if (err == 0) return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0) abort();
}

ResultValue<std::unique_ptr<RenderSystem>> RenderSystem::Create()
{
    RenderSystemCreateInfo ci;
    auto vulkanInstanceRV = VulkanInstance::CreateVulkanInstance();
    if (vulkanInstanceRV.result != GraphicsResult::Ok)
    {
        EZLOG("Failed to create VulkanInstance");
        return vulkanInstanceRV.result;
    }
    ci.vulkanInstance = std::move(vulkanInstanceRV.value);

    auto vulkanDeviceRV = VulkanDevice::CreateVulkanDevice(ci.vulkanInstance->GetInstance());
    if (vulkanDeviceRV.result != GraphicsResult::Ok)
    {
        EZLOG("Failed to create VulkanDevice");
        return vulkanDeviceRV.result;
    }
    ci.vulkanDevice = std::move(vulkanDeviceRV.value);

    auto vulkanSwapchainRV =
        VulkanSwapchain::CreateVulkanSwapchain({ ci.vulkanDevice->GetDevice(),
                                                 ci.vulkanDevice->GetPhysicalDevice(),
                                                 ci.vulkanDevice->GetSurface(),
                                                 ci.vulkanDevice->GetWindow(),
                                                 ci.vulkanDevice->GetQueueFamilyIndices() });
    if (vulkanSwapchainRV.result != GraphicsResult::Ok)
    {
        EZLOG("Failed to create VulkanSwapchain");
        return vulkanSwapchainRV.result;
    }
    ci.vulkanSwapchain = std::move(vulkanSwapchainRV.value);

    vk::SemaphoreCreateInfo semaphoreInfo = {};

    if (ci.vulkanDevice->GetDevice().createSemaphore(
            &semaphoreInfo, nullptr, &ci.frameSemaphores.imageAvailableSemaphore) !=
            vk::Result::eSuccess ||
        ci.vulkanDevice->GetDevice().createSemaphore(
            &semaphoreInfo, nullptr, &ci.frameSemaphores.renderFinishedSemaphore) !=
            vk::Result::eSuccess)
    {
        EZLOG("Failed to create FrameSemaphores!");
        return GraphicsResult::Error;
    }

    auto vulkanRenderPassRV = VulkanRenderPass::CreateRenderPass(
        { ci.vulkanDevice->GetDevice(), ci.vulkanSwapchain->GetInfo().imageFormat });
    if (vulkanRenderPassRV.result != GraphicsResult::Ok)
    {
        EZLOG("Failed to create VulkanRenderPass");
        return vulkanRenderPassRV.result;
    }
    ci.vulkanRenderPass = std::move(vulkanRenderPassRV.value);

    GraphicsResult framebuffersResult = ci.vulkanSwapchain->CreateFramebuffersForRenderPass(
        ci.vulkanRenderPass->GetRenderPass());
    if (framebuffersResult != GraphicsResult::Ok)
    {
        EZLOG("Failed to create Framebuffers");
        return framebuffersResult;
    }

    auto dsLayoutPtr = CreateDescriptorSetLayout(ci.vulkanDevice->GetDevice());
    if (!dsLayoutPtr.has_value())
    {
        EZLOG("Failed to create default vk::DescriptorSetLayout");
        return GraphicsResult::Error;
    }
    ci.descriptorSetLayout = *dsLayoutPtr;

    // create material image sampler descriptor sets
    // clang-format off
    const std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings = {
        { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr },
        { 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr },
        { 2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr },
        { 3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr },
        { 4, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr },
    };
    // clang-format on
    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
    descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
    descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    auto rv =
        ci.vulkanDevice->GetDevice().createDescriptorSetLayout(descriptorSetLayoutCI, nullptr);
    if (rv.result == vk::Result::eSuccess) { ci.samplersDescriptorSetLayout = rv.value; }
    else
    {
        EZLOG("Failed to create samplers DescriptorSetLayout");
        return GraphicsResult::Error;
    }

    ci.vulkanPipelineManager =
        std::make_unique<VulkanPipelineManager>(ci.vulkanDevice->GetDevice());

    ci.commandBuffers = CreateCommandBuffers(ci.vulkanDevice->GetDevice(),
                                             ci.vulkanDevice->GetGraphicsCommandPool(),
                                             ci.vulkanSwapchain->GetInfo());
    if (ci.commandBuffers.empty())
    {
        EZLOG("Failed to create command buffers");
        return GraphicsResult::Error;
    }

    if (!InitializeImGui(ci))
    {
        EZLOG("Failed to initialize ImGui");
        return GraphicsResult::Error;
    }

    return { GraphicsResult::Ok, std::make_unique<RenderSystem>(ci) };
}

RenderSystem::RenderSystem(RenderSystemCreateInfo& ci)
    : vulkanInstance(std::move(ci.vulkanInstance))
    , vulkanDevice(std::move(ci.vulkanDevice))
    , vulkanSwapchain(std::move(ci.vulkanSwapchain))
    , vulkanRenderPass(std::move(ci.vulkanRenderPass))
    , vulkanPipelineManager(std::move(ci.vulkanPipelineManager))
    , descriptorSetLayout(std::move(ci.descriptorSetLayout))
    , samplersDescriptorSetLayout(std::move(ci.samplersDescriptorSetLayout))
    , frameSemaphores(std::move(ci.frameSemaphores))
    , commandBuffers(std::move(ci.commandBuffers))
{
}

std::optional<vk::DescriptorSetLayout> RenderSystem::CreateDescriptorSetLayout(
    vk::Device vkDevice)
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
    if (result != vk::Result::eSuccess)
    {
        EZLOG("Failed to create descriptor set layout!");
        return {};
    }
    return dsLayout;
}

std::vector<vk::CommandBuffer> RenderSystem::CreateCommandBuffers(
    vk::Device logicalDevice,
    vk::CommandPool graphicsCommandPool,
    ez::VulkanSwapchainInfo& swapchainInfo)
{
    std::vector<vk::CommandBuffer> commandBuffers(swapchainInfo.framebuffers.size());

    vk::CommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandPool = graphicsCommandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (logicalDevice.allocateCommandBuffers(&allocInfo, commandBuffers.data()) !=
        vk::Result::eSuccess)
    {
        EZLOG("failed to allocate command buffers!");
        return {};
    }

    return commandBuffers;
}

bool RenderSystem::InitializeImGui(const RenderSystemCreateInfo& ci)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForVulkan(ci.vulkanDevice->GetWindow());

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = ci.vulkanInstance->GetInstance();
    init_info.PhysicalDevice = ci.vulkanDevice->GetPhysicalDevice();
    init_info.Device = ci.vulkanDevice->GetDevice();
    init_info.QueueFamily = ci.vulkanDevice->GetQueueFamilyIndices().graphicsFamily;
    init_info.Queue = ci.vulkanDevice->GetGraphicsQueue();
    init_info.PipelineCache = nullptr;
    init_info.DescriptorPool = ci.vulkanDevice->GetDescriptorPool();
    init_info.Allocator = nullptr;
    init_info.MinImageCount = 2;  // todo: config
    init_info.ImageCount = static_cast<uint32_t>(ci.vulkanSwapchain->GetInfo().images.size());
    init_info.CheckVkResultFn = check_vk_result_imgui;

    ImGui_ImplVulkan_Init(&init_info, ci.vulkanRenderPass->GetRenderPass());

    // Upload ImGui Fonts
    {
        vk::Device device = ci.vulkanDevice->GetDevice();
        vk::CommandPool command_pool = ci.vulkanDevice->GetGraphicsCommandPool();
        vk::CommandBuffer command_buffer = ci.commandBuffers.front();

        device.resetCommandPool(command_pool, vk::CommandPoolResetFlagBits(0));

        vk::CommandBufferBeginInfo begin_info = {};
        begin_info.flags |= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        CheckVkResult(command_buffer.begin(begin_info));

        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

        vk::SubmitInfo end_info = {};
        end_info.commandBufferCount = 1;
        end_info.pCommandBuffers = &command_buffer;
        CheckVkResult(command_buffer.end());

        if (ci.vulkanDevice->GetGraphicsQueue().submit(1, &end_info, nullptr) !=
            vk::Result::eSuccess)
        {
            EZASSERT(false, "Failed to submit draw command buffer (imgui upload fonts)!");
            return false;
        }

        CheckVkResult(device.waitIdle());
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    return true;
}

void RenderSystem::CleanupTotalPipeline()
{
    EZASSERT(vulkanInstance);
    EZASSERT(vulkanDevice);

    vk::Device logicalDevice = vulkanDevice->GetDevice();
    CheckVkResult(logicalDevice.waitIdle());

    vulkanSwapchain.reset();

    logicalDevice.freeCommandBuffers(vulkanDevice->GetGraphicsCommandPool(),
                                     static_cast<uint32_t>(commandBuffers.size()),
                                     commandBuffers.data());
    commandBuffers = {};

    vulkanPipelineManager.reset();
    vulkanRenderPass.reset();
}

void RenderSystem::RecreateTotalPipeline()
{
    CleanupTotalPipeline();

    // todo: remove duplication with CreateRenderSystem
    auto vulkanSwapchainRV =
        VulkanSwapchain::CreateVulkanSwapchain({ vulkanDevice->GetDevice(),
                                                 vulkanDevice->GetPhysicalDevice(),
                                                 vulkanDevice->GetSurface(),
                                                 vulkanDevice->GetWindow(),
                                                 vulkanDevice->GetQueueFamilyIndices() });
    if (vulkanSwapchainRV.result != GraphicsResult::Ok)
    {
        EZLOG("Failed to Recreate VulkanSwapchain");
        return;
    }
    vulkanSwapchain = std::move(vulkanSwapchainRV.value);

    auto vulkanRenderPassRV = VulkanRenderPass::CreateRenderPass(
        { vulkanDevice->GetDevice(), vulkanSwapchain->GetInfo().imageFormat });
    if (vulkanRenderPassRV.result != GraphicsResult::Ok)
    {
        EZLOG("Failed to Recreate VulkanRenderPass");
        return;
    }
    vulkanRenderPass = std::move(vulkanRenderPassRV.value);

    GraphicsResult framebuffersResult =
        vulkanSwapchain->CreateFramebuffersForRenderPass(vulkanRenderPass->GetRenderPass());
    if (framebuffersResult != GraphicsResult::Ok)
    {
        EZLOG("Failed to Recreate Framebuffers");
        return;
    }

    EZASSERT(!vulkanPipelineManager);

    vulkanPipelineManager = std::make_unique<VulkanPipelineManager>(GetDevice());

    EZASSERT(commandBuffers.empty());
    commandBuffers = CreateCommandBuffers(
        GetDevice(), vulkanDevice->GetGraphicsCommandPool(), GetSwapchainInfo());

    needRecreateResources = true;
}

void RenderSystem::UpdateGlobalUniforms(std::shared_ptr<Scene> scene,
                                        const std::unique_ptr<Camera>& camera)
{
    vk::Device logicalDevice = vulkanDevice->GetDevice();

    void* data;
    GlobalUBO ubo = { camera->GetViewMatrix(),
                      camera->GetProjectionMatrix(),
                      camera->GetViewProjectionMatrix() };

    for (Model& model : scene->GetModelsMutable())
    {
        CheckVkResult(logicalDevice.mapMemory(
            model.uniformBufferMemory, 0, sizeof(ubo), vk::MemoryMapFlags(), &data));
        memcpy(data, &ubo, sizeof(ubo));
        logicalDevice.unmapMemory(model.uniformBufferMemory);
    }
}

void RenderSystem::PrepareToRender(std::shared_ptr<Scene> scene)
{
    if (needRecreateResources)
    {
        scene->SetReadyToRender(false);
        needRecreateResources = false;
    }
    if (scene->ReadyToRender()) { return; }

    // todo: cleanup old scene models
    std::vector<Model>& sceneModels = scene->GetModelsMutable();
    bool modelsCreateSuccess = true;
    for (Model& model : sceneModels)
    {
        model.SetLogicalDevice(GetDevice());
        modelsCreateSuccess |= model.CreateVertexBuffers(
            GetPhysicalDevice(), GetGraphicsQueue(), vulkanDevice->GetGraphicsCommandPool());
        modelsCreateSuccess |= model.CreateDescriptorSet(
            vulkanDevice->GetDescriptorPool(), descriptorSetLayout, sizeof(GlobalUBO));

        for (Texture& tex : model.textures)
        {
            if (!tex.IsLoadedToGPU())
            {
                modelsCreateSuccess |= tex.LoadToGpu(GetDevice(),
                                                     GetPhysicalDevice(),
                                                     vulkanDevice->GetGraphicsQueue(),
                                                     vulkanDevice->GetGraphicsCommandPool());
            }
        }

        for (Material& material : model.materials)
        {
            if (material.textures.baseColor == nullptr) { continue; }
            vk::DescriptorSetAllocateInfo descriptorSetAllocInfo{};
            descriptorSetAllocInfo.descriptorPool = vulkanDevice->GetDescriptorPool();
            descriptorSetAllocInfo.pSetLayouts = &samplersDescriptorSetLayout;
            descriptorSetAllocInfo.descriptorSetCount = 1;
            CheckVkResult(GetDevice().allocateDescriptorSets(&descriptorSetAllocInfo,
                                                             &material.descriptorSet));
            // todo: handle missing textures - create white and black tex
            vk::DescriptorImageInfo defaultWhiteTexture =
                material.textures.baseColor->descriptor;
            vk::DescriptorImageInfo defaultBlackTexture =
                material.textures.baseColor->descriptor;
            const std::vector<vk::DescriptorImageInfo> imageDescriptors = {
                material.textures.baseColor ? material.textures.baseColor->descriptor
                                            : defaultWhiteTexture,
                material.textures.metallicRoughness
                    ? material.textures.metallicRoughness->descriptor
                    : defaultBlackTexture,
                material.textures.normal ? material.textures.normal->descriptor
                                         : defaultWhiteTexture,
                material.textures.occlusion ? material.textures.occlusion->descriptor
                                            : defaultWhiteTexture,
                material.textures.emission ? material.textures.emission->descriptor
                                           : defaultBlackTexture
            };

            std::array<vk::WriteDescriptorSet, 5> writeDescriptorSets{};
            for (size_t i = 0; i < imageDescriptors.size(); i++)
            {
                writeDescriptorSets[i].descriptorType =
                    vk::DescriptorType::eCombinedImageSampler;
                writeDescriptorSets[i].descriptorCount = 1;
                writeDescriptorSets[i].dstSet = material.descriptorSet;
                writeDescriptorSets[i].dstBinding = static_cast<uint32_t>(i);
                writeDescriptorSets[i].pImageInfo = &imageDescriptors[i];
            }

            GetDevice().updateDescriptorSets(static_cast<uint32_t>(writeDescriptorSets.size()),
                                             writeDescriptorSets.data(),
                                             0,
                                             nullptr);
        }

        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = {
            model.descriptorSetLayout, samplersDescriptorSetLayout
            // todo: global UBO as separate descriptor set
            // todo: textures descriptor set
        };
        auto vulkanGraphicsPipelineRV = vulkanPipelineManager->CreateGraphicsPipeline(
            GetSwapchainInfo().extent, vulkanRenderPass->GetRenderPass(), descriptorSetLayouts);
        if (vulkanGraphicsPipelineRV.result != GraphicsResult::Ok)
        {
            EZASSERT(false, "Failed to create graphics pipeline for model");
            modelsCreateSuccess = false;
        }
        else
        {
            model.graphicsPipeline = vulkanGraphicsPipelineRV.value;
        }
    }
    EZASSERT(modelsCreateSuccess);
    scene->SetReadyToRender(true);
}

static void DrawNodeRecursive(const Model& model,
                              const std::unique_ptr<Node>& node,
                              vk::CommandBuffer& curCb)
{
    if (node->mesh)
    {
        for (const std::unique_ptr<Primitive>& primitive : node->mesh->primitives)
        {
            curCb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                     model.graphicsPipeline->GetPipelineLayout(),
                                     0,
                                     { model.descriptorSet, primitive->material.descriptorSet },
                                     {});

            curCb.pushConstants(model.graphicsPipeline->GetPipelineLayout(),
                                vk::ShaderStageFlagBits::eVertex,
                                0,
                                Mesh::PushConstantsBlockSize,
                                &node->mesh->pushConstantsBlock);

            curCb.drawIndexed(primitive->indexCount, 1, 0, 0, 0);
        }
    }

    for (const std::unique_ptr<Node>& child : node->children)
    {
        DrawNodeRecursive(model, child, curCb);
    }
}

void RenderSystem::Draw(const std::unique_ptr<View>& view,
                        const std::unique_ptr<Camera>& camera)
{
    std::shared_ptr<Scene> scene = view->GetScene();
    UpdateGlobalUniforms(scene, camera);

    vk::Device logicalDevice = vulkanDevice->GetDevice();

    const VulkanSwapchainInfo& swapchainInfo = vulkanSwapchain->GetInfo();
    vk::Queue graphicsQueue = vulkanDevice->GetGraphicsQueue();
    vk::Queue presentQueue = vulkanDevice->GetPresentQueue();

    uint32_t imageIndex;
    vk::Result result =
        logicalDevice.acquireNextImageKHR(swapchainInfo.swapchain,
                                          std::numeric_limits<uint64_t>::max(),
                                          frameSemaphores.imageAvailableSemaphore,
                                          nullptr,
                                          &imageIndex);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        RecreateTotalPipeline();
        return;
    }
    else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        EZASSERT(false, "Failed to acquire swapchain image!");
    }

    vk::SubmitInfo submitInfo = {};

    vk::Semaphore waitSemaphores[] = { frameSemaphores.imageAvailableSemaphore };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    EZASSERT((curFrameIndex < commandBuffers.size()));
    vk::CommandBuffer curCb = commandBuffers.at(curFrameIndex);

    std::array<float, 4> clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    vk::CommandBufferBeginInfo beginInfo = {};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;

    CheckVkResult(curCb.begin(&beginInfo));

    vk::RenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.renderPass = vulkanRenderPass->GetRenderPass();
    renderPassInfo.framebuffer = swapchainInfo.framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
    renderPassInfo.renderArea.extent = swapchainInfo.extent;

    std::array<vk::ClearValue, 2> clearValues = { vk::ClearColorValue(clearColor),
                                                  vk::ClearDepthStencilValue(1.0f, 0) };
    renderPassInfo.clearValueCount = clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();

    curCb.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

    for (const Model& model : scene->GetModelsMutable())
    {
        if (!model.graphicsPipeline)
        {
            EZASSERT(false, "Invalid Model Graphics Pipeline");
            continue;
        }
        curCb.bindPipeline(vk::PipelineBindPoint::eGraphics,
                           model.graphicsPipeline->GetPipeline());

        vk::Buffer vertexBuffers[] = { model.vertexBuffer };
        vk::DeviceSize offsets[] = { 0 };

        curCb.bindVertexBuffers(0, 1, vertexBuffers, offsets);
        curCb.bindIndexBuffer(model.indexBuffer, 0, vk::IndexType::eUint32);

        for (const std::unique_ptr<Node>& node : model.nodes)
        {
            DrawNodeRecursive(model, node, curCb);
        }
    }

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), curCb);

    curCb.endRenderPass();

    if (curCb.end() != vk::Result::eSuccess)
    {
        EZASSERT(false, "failed to record command buffer!");
    }

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &curCb;
    curFrameIndex = (curFrameIndex + 1) % commandBuffers.size();

    vk::Semaphore signalSemaphores[] = { frameSemaphores.renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (graphicsQueue.submit(1, &submitInfo, nullptr) != vk::Result::eSuccess)
    {
        EZASSERT(false, "Failed to submit draw command buffer!");
    }

    vk::PresentInfoKHR presentInfo = {};

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    vk::SwapchainKHR swapchains[] = { swapchainInfo.swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;

    presentInfo.pImageIndices = &imageIndex;

    result = presentQueue.presentKHR(&presentInfo);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
    {
        RecreateTotalPipeline();
    }
    else if (result != vk::Result::eSuccess)
    {
        EZASSERT(false, "Failed to present swapchain image!");
    }

    CheckVkResult(presentQueue.waitIdle());
}

RenderSystem::~RenderSystem()
{
    // todo: cleanup all scenes models?

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    CleanupTotalPipeline();

    vk::Device logicalDevice = vulkanDevice->GetDevice();
    CheckVkResult(logicalDevice.waitIdle());

    logicalDevice.destroyDescriptorSetLayout(descriptorSetLayout);

    logicalDevice.destroySemaphore(frameSemaphores.renderFinishedSemaphore);
    logicalDevice.destroySemaphore(frameSemaphores.imageAvailableSemaphore);

    vulkanDevice.reset();
    vulkanInstance.reset();
}

}  // namespace ez
