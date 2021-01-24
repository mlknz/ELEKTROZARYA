#include "vulkan_pipeline_manager.hpp"

#include "core/log_assert.hpp"
#include "core/scene/mesh.hpp"

namespace ez
{
VulkanPipelineManager::VulkanPipelineManager(vk::Device aLogicalDevice)
    : logicalDevice(aLogicalDevice)
{
}

VulkanPipelineManager::~VulkanPipelineManager() {}

ResultValue<std::shared_ptr<VulkanGraphicsPipeline>>
VulkanPipelineManager::CreateGraphicsPipeline(vk::Extent2D swapchainExtent,
                                              vk::RenderPass renderPass,
                                              vk::DescriptorSetLayout descriptorSetLayout)
{
    auto vulkanGraphicsPipeline = VulkanGraphicsPipeline::CreateVulkanGraphicsPipeline(
        logicalDevice, swapchainExtent, renderPass, descriptorSetLayout);
    if (vulkanGraphicsPipeline)
    {
        return { GraphicsResult::Ok, std::move(vulkanGraphicsPipeline) };
    }

    EZASSERT(false, "Failed to create VulkanGraphicsPipeline");
    return GraphicsResult::Error;
}
}  // namespace ez
