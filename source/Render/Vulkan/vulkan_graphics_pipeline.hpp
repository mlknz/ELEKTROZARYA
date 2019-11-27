#pragma once

#include <vector>
#include "render/vulkan_include.hpp"

namespace Ride {

class GraphicsPipeline
{
public:
    GraphicsPipeline(vk::Device logicalDevice, vk::Extent2D swapchainExtent,
                     vk::RenderPass renderPass, vk::DescriptorSetLayout descriptorSetLayout);
    ~GraphicsPipeline();

    vk::Pipeline GetPipeline() { return graphicsPipeline; }
    vk::PipelineLayout GetLayout() { return pipelineLayout; }

    bool Ready() const { return ready; }

private:
    vk::ShaderModule createShaderModule(const std::vector<char>& code);
    bool CreateGraphicsPipeline(vk::Extent2D swapchainExtent, vk::RenderPass renderPass, vk::DescriptorSetLayout descriptorSetLayout);

    vk::Device logicalDevice;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline graphicsPipeline;

    bool ready = false;
};

}

