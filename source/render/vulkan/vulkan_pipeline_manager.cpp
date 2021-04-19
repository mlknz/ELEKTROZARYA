#include "vulkan_pipeline_manager.hpp"

#include "core/log_assert.hpp"

namespace ez
{
VulkanPipelineManager::VulkanPipelineManager(vk::Device aLogicalDevice)
    : logicalDevice(aLogicalDevice)
{
}

VulkanPipelineManager::~VulkanPipelineManager() {}

ResultValue<std::shared_ptr<VulkanGraphicsPipeline>>
VulkanPipelineManager::CreateGraphicsPipeline(
    vk::Extent2D swapchainExtent,
    vk::RenderPass renderPass,
    const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts,
    VertexLayout vertexLayout,
    const std::string& vertexShaderName,
    const std::string& fragmentShaderName)
{
    auto vulkanGraphicsPipeline =
        VulkanGraphicsPipeline::CreateVulkanGraphicsPipeline(logicalDevice,
                                                             swapchainExtent,
                                                             renderPass,
                                                             descriptorSetLayouts,
                                                             vertexLayout,
                                                             vertexShaderName,
                                                             fragmentShaderName);
    if (vulkanGraphicsPipeline)
    {
        return { GraphicsResult::Ok, std::move(vulkanGraphicsPipeline) };
    }

    EZASSERT(false, "Failed to create VulkanGraphicsPipeline");
    return GraphicsResult::Error;
}
}  // namespace ez
