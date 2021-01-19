#pragma once

#include "render/graphics_result.hpp"
#include "render/vulkan_include.hpp"

namespace ez
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

    static ResultValue<std::unique_ptr<VulkanRenderPass>> CreateRenderPass(
        VulkanRenderPassCreateInfo);

   private:
    vk::Device logicalDevice;
    vk::RenderPass renderPass;
};

}  // namespace ez
