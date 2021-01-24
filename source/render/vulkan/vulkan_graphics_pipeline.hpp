#pragma once

#include <memory>
#include <vector>

#include "render/vulkan_include.hpp"

namespace ez
{
class VulkanGraphicsPipeline
{
   public:
    VulkanGraphicsPipeline(VulkanGraphicsPipeline&& other);
    ~VulkanGraphicsPipeline();

    vk::Pipeline GetPipeline() const { return graphicsPipeline; }
    vk::PipelineLayout GetPipelineLayout() const { return pipelineLayout; }

    static std::shared_ptr<VulkanGraphicsPipeline> CreateVulkanGraphicsPipeline(
        vk::Device logicalDevice,
        vk::Extent2D swapchainExtent,
        vk::RenderPass renderPass,
        vk::DescriptorSetLayout descriptorSetLayout);

   private:
    VulkanGraphicsPipeline(vk::Device aLogicalDevice);

    vk::ShaderModule createShaderModule(const std::vector<char>& code);
    bool CreateGraphicsPipeline(vk::Extent2D swapchainExtent,
                                vk::RenderPass renderPass,
                                vk::DescriptorSetLayout descriptorSetLayout);

    vk::Device logicalDevice;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline graphicsPipeline;
};

}  // namespace ez
