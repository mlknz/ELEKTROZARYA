#pragma once

#include <memory>
#include <vector>

#include "render/graphics_result.hpp"
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
        vk::DescriptorSetLayout descriptorSetLayout);

   private:
    vk::Device logicalDevice;
};
}  // namespace ez
