#include "texture_sampler.hpp"

#include "core/log_assert.hpp"

namespace ez
{
static vk::Filter GetVkFilterMode(int32_t filterMode)
{
    switch (filterMode)
    {
        case 9728:
            return vk::Filter::eNearest;
        case 9984:
            return vk::Filter::eNearest;
        case 9985:
            return vk::Filter::eNearest;
        case 9729:
            return vk::Filter::eLinear;
        case 9986:
            return vk::Filter::eLinear;
        case 9987:
            return vk::Filter::eLinear;
        default:
            EZASSERT(false);
            return vk::Filter::eNearest;
    }
}

static vk::SamplerAddressMode GetVkWrapMode(int32_t wrapMode)
{
    switch (wrapMode)
    {
        case 10497:
            return vk::SamplerAddressMode::eRepeat;
        case 33071:
            return vk::SamplerAddressMode::eClampToEdge;
        case 33648:
            return vk::SamplerAddressMode::eMirroredRepeat;
        default:
            EZASSERT(false);
            return vk::SamplerAddressMode::eRepeat;
    }
}

TextureSampler TextureSampler::FromGltfSampler(const tinygltf::Sampler& gltfSampler)
{
    TextureSampler sampler{};
    sampler.minFilter = GetVkFilterMode(gltfSampler.minFilter);
    sampler.magFilter = GetVkFilterMode(gltfSampler.magFilter);
    sampler.addressModeU = GetVkWrapMode(gltfSampler.wrapS);
    sampler.addressModeV = GetVkWrapMode(gltfSampler.wrapT);
    sampler.addressModeW = sampler.addressModeV;
    return sampler;
}
}  // namespace ez
