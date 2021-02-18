#pragma once
#include "render/graphics_result.hpp"
#include "render/vulkan_include.hpp"

namespace ez
{
class VulkanOneTimeCommandBuffer
{
   public:
    static VulkanOneTimeCommandBuffer Start(vk::Device logicalDevice,
                                            vk::CommandPool commandPool)
    {
        VulkanOneTimeCommandBuffer oneTimeCB;
        oneTimeCB.logicalDevice = logicalDevice;
        oneTimeCB.commandPool = commandPool;

        vk::CommandBufferAllocateInfo allocInfo = {};
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        vk::CommandBufferBeginInfo beginInfo = {};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        CheckVkResult(
            logicalDevice.allocateCommandBuffers(&allocInfo, &oneTimeCB.commandBuffer));
        CheckVkResult(oneTimeCB.commandBuffer.begin(&beginInfo));

        return oneTimeCB;
    }

    void EndSubmitAndWait(vk::Queue queue)
    {
        CheckVkResult(commandBuffer.end());

        vk::SubmitInfo submitInfo = vk::SubmitInfo{};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        CheckVkResult(queue.submit(1, &submitInfo, nullptr));
        CheckVkResult(queue.waitIdle());
    }

    vk::CommandBuffer GetCommandBuffer() const { return commandBuffer; }

    ~VulkanOneTimeCommandBuffer()
    {
        logicalDevice.freeCommandBuffers(commandPool, 1, &commandBuffer);
    }

   private:
    VulkanOneTimeCommandBuffer() = default;

    vk::Device logicalDevice;
    vk::CommandPool commandPool;
    vk::CommandBuffer commandBuffer;
};
}  // namespace ez
