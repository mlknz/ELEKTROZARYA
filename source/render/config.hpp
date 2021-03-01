#pragma once

#include <map>

#include "render/vulkan_include.hpp"

namespace ez
{
namespace Config
{
constexpr int WindowWidth = 1024;
constexpr int WindowHeight = 768;

constexpr uint32_t EnvCubemapSize = 1024;

constexpr uint32_t MaxDescriptorSetsCount = 1000;

const std::map<vk::DescriptorType, uint32_t> VulkanDescriptorPoolSizes = {
    { vk::DescriptorType::eUniformBuffer, 32 },
    { vk::DescriptorType::eSampler, 1000 },
    { vk::DescriptorType::eCombinedImageSampler, 1000 },
    { vk::DescriptorType::eSampledImage, 1000 },
    { vk::DescriptorType::eStorageImage, 1000 },
    { vk::DescriptorType::eStorageBuffer, 1000 },
    { vk::DescriptorType::eUniformBufferDynamic, 1000 },
    { vk::DescriptorType::eUniformBufferDynamic, 1000 },
    { vk::DescriptorType::eStorageBufferDynamic, 1000 },
};

constexpr vk::Format DepthAttachmentFormat = vk::Format::eD32Sfloat;

constexpr bool dumpGlslSources = false;

// /////////////////// RUNTIME //////////////////////

extern bool msaa8xEnabled;
}  // namespace Config
}  // namespace ez
