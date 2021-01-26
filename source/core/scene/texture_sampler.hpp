#pragma once

#include "core/scene/tiny_gltf_include.hpp"
#include "render/vulkan_include.hpp"

namespace ez
{
struct TextureSampler final
{
    static TextureSampler FromGltfSampler(const tinygltf::Sampler& gltfSampler);

    vk::Filter magFilter;
    vk::Filter minFilter;
    vk::SamplerAddressMode addressModeU;
    vk::SamplerAddressMode addressModeV;
    vk::SamplerAddressMode addressModeW;
};
}  // namespace ez
