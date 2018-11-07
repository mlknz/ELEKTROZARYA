#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace Ride {

class GraphicsPipeline
{
public:
    GraphicsPipeline(VkDevice logicalDevice, VkExtent2D swapchainExtent,
                     VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout);
    ~GraphicsPipeline();

    VkPipeline GetPipeline() { return graphicsPipeline; }
    VkPipelineLayout GetLayout() { return pipelineLayout; }

    bool Ready() const { return ready; }

private:
    VkShaderModule createShaderModule(const std::vector<char>& code);
    bool CreateGraphicsPipeline(VkExtent2D swapchainExtent, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout);

    VkDevice logicalDevice;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    bool ready = false;
};

}

