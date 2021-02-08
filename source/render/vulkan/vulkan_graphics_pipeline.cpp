#include "vulkan_graphics_pipeline.hpp"

#include "core/file_utils.hpp"
#include "core/log_assert.hpp"
#include "render/highlevel/mesh.hpp"
#include "render/vulkan/vulkan_shader_compiler.hpp"

namespace ez
{
VulkanGraphicsPipeline::VulkanGraphicsPipeline(vk::Device aLogicalDevice)
    : logicalDevice(aLogicalDevice)
{
}

VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanGraphicsPipeline&& other)
{
    logicalDevice = other.logicalDevice;
    pipelineLayout = other.pipelineLayout;
    graphicsPipeline = other.graphicsPipeline;

    other.logicalDevice = nullptr;
    other.pipelineLayout = nullptr;
    other.graphicsPipeline = nullptr;
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
    if (logicalDevice)
    {
        logicalDevice.destroyPipeline(graphicsPipeline);
        logicalDevice.destroyPipelineLayout(pipelineLayout);
    }
}

std::shared_ptr<VulkanGraphicsPipeline> VulkanGraphicsPipeline::CreateVulkanGraphicsPipeline(
    vk::Device logicalDevice,
    vk::Extent2D swapchainExtent,
    vk::RenderPass renderPass,
    const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)
{
    VulkanGraphicsPipeline obj{ logicalDevice };
    if (obj.CreateGraphicsPipeline(swapchainExtent, renderPass, descriptorSetLayouts))
    {
        return std::make_shared<VulkanGraphicsPipeline>(std::move(obj));
    }
    return {};
}

vk::ShaderModule VulkanGraphicsPipeline::CreateShaderModule(const std::vector<uint32_t>& code)
{
    vk::ShaderModuleCreateInfo createInfo = {};
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    vk::ShaderModule shaderModule;
    if (logicalDevice.createShaderModule(&createInfo, nullptr, &shaderModule) !=
        vk::Result::eSuccess)
    {
        EZASSERT(false, "failed to create shader module!");
    }

    return shaderModule;
}

bool VulkanGraphicsPipeline::CreateGraphicsPipeline(
    vk::Extent2D swapchainExtent,
    vk::RenderPass renderPass,
    const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts)
{
    const std::vector<uint32_t> vertShaderCode =
        SpirVShaderCompiler::CompileFromGLSL("../source/shaders/shader.vert");
    const std::vector<uint32_t> fragShaderCode =
        SpirVShaderCompiler::CompileFromGLSL("../source/shaders/shader.frag");

    vk::ShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
    vk::ShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo,
                                                         fragShaderStageInfo };

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};

    auto bindingDescription = ez::Vertex::getBindingDescription();
    auto attributeDescriptions = ez::Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    vk::Viewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchainExtent.width);
    viewport.height = static_cast<float>(swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor = {};
    scissor.offset = vk::Offset2D{ 0, 0 };
    scissor.extent = swapchainExtent;

    vk::PipelineViewportStateCreateInfo viewportState = {};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    vk::PipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;

    vk::PipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};

    colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    vk::PipelineDepthStencilStateCreateInfo depthStencilState = {};
    depthStencilState.depthWriteEnable = true;
    depthStencilState.depthTestEnable = true;
    depthStencilState.depthCompareOp = vk::CompareOp::eLess;

    static_assert(Mesh::PushConstantsBlockSize <= 128);
    vk::PushConstantRange pushConstantRange(
        vk::ShaderStageFlagBits::eVertex, 0, Mesh::PushConstantsBlockSize);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
        vk::PipelineLayoutCreateFlags{},
        static_cast<uint32_t>(descriptorSetLayouts.size()),
        descriptorSetLayouts.data(),
        1,
        &pushConstantRange);

    if (logicalDevice.createPipelineLayout(&pipelineLayoutInfo, nullptr, &pipelineLayout) !=
        vk::Result::eSuccess)
    {
        EZLOG("Failed to create pipeline layout!");
        return false;
    }

    vk::GraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencilState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (logicalDevice.createGraphicsPipelines(
            nullptr, 1, &pipelineInfo, nullptr, &graphicsPipeline) != vk::Result::eSuccess)
    {
        EZLOG("Failed to create graphics pipeline!");
        return false;
    }

    logicalDevice.destroyShaderModule(vertShaderModule, nullptr);
    logicalDevice.destroyShaderModule(fragShaderModule, nullptr);
    return true;
}
}  // namespace ez
