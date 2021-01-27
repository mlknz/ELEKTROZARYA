#pragma once

#include "render/vulkan_include.hpp"

namespace ez
{
struct TextureSampler final
{
    static TextureSampler FromGltfSampler(int32_t gltfMagFilter,
                                          int32_t gltfMinFilter,
                                          int32_t gltfWrapS,
                                          int32_t gltfWrapT);

    vk::Filter magFilter = vk::Filter::eLinear;
    vk::Filter minFilter = vk::Filter::eLinear;
    vk::SamplerAddressMode addressModeU = vk::SamplerAddressMode::eRepeat;
    vk::SamplerAddressMode addressModeV = vk::SamplerAddressMode::eRepeat;
    vk::SamplerAddressMode addressModeW = vk::SamplerAddressMode::eRepeat;
};
}  // namespace ez
