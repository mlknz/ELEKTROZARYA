#include "texture_sampler.hpp"

#include "core/log_assert.hpp"

namespace ez
{
static vk::Filter GetVkFilterMode(int32_t filterMode)
{
    switch (filterMode)  // values from tiny_gltf.h
    {
        case 9728:  // TINYGLTF_TEXTURE_FILTER_NEAREST
            return vk::Filter::eNearest;
        case 9984:  // TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST
            return vk::Filter::eNearest;
        case 9985:  // TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST
            return vk::Filter::eNearest;
        case 9729:  // TINYGLTF_TEXTURE_FILTER_LINEAR
            return vk::Filter::eLinear;
        case 9986:  // TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR
            return vk::Filter::eLinear;
        case 9987:  // TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR
            return vk::Filter::eLinear;
        default:
            // EZASSERT(false);
            return vk::Filter::eNearest;
    }
}

static vk::SamplerAddressMode GetVkWrapMode(int32_t wrapMode)
{
    switch (wrapMode)  // values from tiny_gltf.h
    {
        case 10497:  // TINYGLTF_TEXTURE_WRAP_REPEAT
            return vk::SamplerAddressMode::eRepeat;
        case 33071:  // TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE
            return vk::SamplerAddressMode::eClampToEdge;
        case 33648:
            return vk::SamplerAddressMode::eMirroredRepeat;
        default:  // TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT
            // EZASSERT(false);
            return vk::SamplerAddressMode::eRepeat;
    }
}

TextureSampler TextureSampler::FromGltfSampler(int32_t gltfMagFilter,
                                               int32_t gltfMinFilter,
                                               int32_t gltfWrapS,
                                               int32_t gltfWrapT)
{
    TextureSampler sampler{};
    sampler.magFilter = GetVkFilterMode(gltfMagFilter);
    sampler.minFilter = GetVkFilterMode(gltfMinFilter);
    sampler.addressModeU = GetVkWrapMode(gltfWrapS);
    sampler.addressModeV = GetVkWrapMode(gltfWrapT);
    sampler.addressModeW = sampler.addressModeV;

    return sampler;
}
}  // namespace ez
