#pragma once

#include <memory>
#include <vector>

#include "render/highlevel/primitive.hpp"
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
        const std::vector<vk::DescriptorSetLayout>& descriptorSetLayout,
        VertexLayout vertexLayout,
        const std::string& vertexShaderName,
        const std::string& fragmentShaderName);

   private:
    VulkanGraphicsPipeline(vk::Device aLogicalDevice);

    vk::ShaderModule CreateShaderModule(const std::vector<uint32_t>& code);
    bool CreateGraphicsPipeline(vk::Extent2D swapchainExtent,
                                vk::RenderPass renderPass,
                                const std::vector<vk::DescriptorSetLayout>& descriptorSetLayout,
                                VertexLayout vertexLayout,
                                const std::string& vertexShaderName,
                                const std::string& fragmentShaderName);

    vk::Device logicalDevice;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline graphicsPipeline;
};

}  // namespace ez
