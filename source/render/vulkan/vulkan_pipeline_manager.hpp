#pragma once

#include <memory>
#include <string>
#include <vector>

#include "render/graphics_result.hpp"
#include "render/highlevel/primitive.hpp"
#include "render/vulkan/vulkan_graphics_pipeline.hpp"

namespace ez
{
class VulkanPipelineManager
{
   public:
    VulkanPipelineManager(vk::Device aLogicalDevice);
    ~VulkanPipelineManager();

    ResultValue<std::shared_ptr<VulkanGraphicsPipeline>> CreateGraphicsPipeline(
        vk::Extent2D swapchainExtent,
        vk::RenderPass renderPass,
        const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts,
        VertexLayout vertexLayout,
        const std::string& vertexShaderName,
        const std::string& fragmentShaderName);

   private:
    vk::Device logicalDevice;
};
}  // namespace ez
