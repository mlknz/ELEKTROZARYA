#include "vulkan_render_pass.hpp"

using namespace ez;

ResultValue<std::unique_ptr<VulkanRenderPass>> VulkanRenderPass::CreateRenderPass(VulkanRenderPassCreateInfo ci)
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

    vk::AttachmentReference colorAttachmentRef = {0, vk::ImageLayout::eColorAttachmentOptimal};

    vk::SubpassDescription subpass = {};
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setColorAttachmentCount(1)
            .setPColorAttachments(&colorAttachmentRef);

    vk::RenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.setAttachmentCount(1)
            .setPAttachments(&colorAttachment)
            .setSubpassCount(1)
            .setPSubpasses(&subpass)
            .setDependencyCount(0);

    vk::Result result = ci.logicalDevice.createRenderPass(&renderPassInfo, nullptr, &renderPass);
    if (result != vk::Result::eSuccess) {
        printf("Failed to create render pass!");
        return GraphicsResult::Error;
    }
    return {GraphicsResult::Ok, std::make_unique<VulkanRenderPass>(ci.logicalDevice, renderPass)};
}

VulkanRenderPass::VulkanRenderPass(vk::Device aLogicalDevice, vk::RenderPass aRenderPass)
    : logicalDevice(aLogicalDevice)
    , renderPass(aRenderPass)
{
}

VulkanRenderPass::~VulkanRenderPass()
{
    logicalDevice.destroyRenderPass(renderPass);
}

