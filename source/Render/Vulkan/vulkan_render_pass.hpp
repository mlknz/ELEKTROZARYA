#pragma once

#include <vulkan_include.hpp>
#include <graphics_result.hpp>

namespace Ride
{
struct VulkanRenderPassCreateInfo
{
    vk::Device logicalDevice;
    vk::Format imageFormat;
};

class VulkanRenderPass
{
public:
    VulkanRenderPass() = delete;
    VulkanRenderPass(const VulkanRenderPass&) = delete;
    VulkanRenderPass(vk::Device aLogicalDevice, vk::RenderPass aRenderPass);

    ~VulkanRenderPass();

    vk::RenderPass& GetRenderPass() { return renderPass; }

    static ResultValue<std::unique_ptr<VulkanRenderPass>> CreateRenderPass(VulkanRenderPassCreateInfo);

private:
    vk::Device logicalDevice;
    vk::RenderPass renderPass;
};

}
