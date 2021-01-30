#include "vulkan_render_pass.hpp"

#include "core/log_assert.hpp"
#include "render/config.hpp"

using namespace ez;

ResultValue<std::unique_ptr<VulkanRenderPass>> VulkanRenderPass::CreateRenderPass(
    VulkanRenderPassCreateInfo ci)
{
    vk::RenderPass renderPass;
    vk::AttachmentDescription colorAttachment = {};
    colorAttachment.setFormat(ci.imageFormat)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentDescription depthAttachment{};
    depthAttachment.setFormat(Config::DepthAttachmentFormat)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::eDepthAttachmentOptimal);

    vk::AttachmentReference colorAttachmentRef = { 0,
                                                   vk::ImageLayout::eColorAttachmentOptimal };

    vk::AttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription subpass = {};
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachmentCount(1)
        .setPColorAttachments(&colorAttachmentRef)
        .setPDepthStencilAttachment(&depthAttachmentRef);

    vk::SubpassDependency dependency = {};
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
                         vk::PipelineStageFlagBits::eEarlyFragmentTests)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
                         vk::PipelineStageFlagBits::eEarlyFragmentTests)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite |
                          vk::AccessFlagBits::eDepthStencilAttachmentWrite);

    std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    vk::RenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.setAttachmentCount(attachments.size())
        .setPAttachments(attachments.data())
        .setSubpassCount(1)
        .setPSubpasses(&subpass)
        .setDependencyCount(1)
        .setDependencies(dependency);

    vk::Result result =
        ci.logicalDevice.createRenderPass(&renderPassInfo, nullptr, &renderPass);
    if (result != vk::Result::eSuccess)
    {
        EZLOG("Failed to create render pass!");
        return GraphicsResult::Error;
    }
    return { GraphicsResult::Ok,
             std::make_unique<VulkanRenderPass>(ci.logicalDevice, renderPass) };
}

VulkanRenderPass::VulkanRenderPass(vk::Device aLogicalDevice, vk::RenderPass aRenderPass)
    : logicalDevice(aLogicalDevice), renderPass(aRenderPass)
{
}

VulkanRenderPass::~VulkanRenderPass() { logicalDevice.destroyRenderPass(renderPass); }
