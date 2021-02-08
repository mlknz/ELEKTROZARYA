#pragma once

#include <glm/glm.hpp>

#include "render/highlevel/texture.hpp"
#include "render/vulkan_include.hpp"

namespace ez
{
enum class BlendMode
{
    eOpaque,
    eAlphaMask,
    eAlphaBlend
};

struct Material
{
    struct PbrTextures
    {
        Texture* baseColor;
        Texture* metallicRoughness;
        Texture* normal;
        Texture* occlusion;
        Texture* emission;
    } textures;

    struct TexCoordSets
    {
        uint8_t baseColor = 0;
        uint8_t metallicRoughness = 0;
        uint8_t specularGlossiness = 0;
        uint8_t normal = 0;
        uint8_t occlusion = 0;
        uint8_t emissive = 0;
    } texCoordSets;

    BlendMode blendMode = BlendMode::eOpaque;
    float alphaCutoff = 0.5f;

    vk::DescriptorSet descriptorSet;
};
}  // namespace ez
