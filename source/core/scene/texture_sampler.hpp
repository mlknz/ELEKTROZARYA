#pragma once

#include "render/vulkan_include.hpp"

namespace tinygltf
{
struct Sampler;
}
namespace ez
{
struct TextureSampler final
{
    static TextureSampler FromGltfSampler(const tinygltf::Sampler& gltfSampler);

    vk::Filter magFilter = vk::Filter::eLinear;
    vk::Filter minFilter = vk::Filter::eLinear;
    vk::SamplerAddressMode addressModeU = vk::SamplerAddressMode::eRepeat;
    vk::SamplerAddressMode addressModeV = vk::SamplerAddressMode::eRepeat;
    vk::SamplerAddressMode addressModeW = vk::SamplerAddressMode::eRepeat;
};
}  // namespace ez
