#pragma once

#include <vector>

#include "render/highlevel/texture_sampler.hpp"
#include "render/vulkan_include.hpp"

namespace ez
{
struct TextureCreationInfo final
{
    static TextureCreationInfo CreateFromData(uint8_t* data,
                                              uint32_t width,
                                              uint32_t height,
                                              uint32_t colorChannelsCount,
                                              uint32_t imageLayersCount,
                                              bool needMips,
                                              const TextureSampler& textureSampler);

    static TextureCreationInfo CreateHdrFromData(float* data,
                                                 uint32_t width,
                                                 uint32_t height,
                                                 uint32_t channelsCount,
                                                 const TextureSampler& textureSampler);

    bool IsValid() const;

    void SetIsCubemap(bool value) { isCubemap = value; }
    bool IsCubemap() const { return isCubemap; }

    vk::Format format = vk::Format::eR8G8B8A8Unorm;
    uint32_t colorChannelsCount = 4;

    // CPU data to load to GPU
    std::vector<uint8_t> buffer;
    TextureSampler textureSampler;

    // other
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t imageLayersCount = 1;
    uint32_t mipLevels = 0;

   private:
    bool isCubemap = false;
};

class Texture
{
   public:
    Texture(TextureCreationInfo&& aCreationInfo) : creationInfo(std::move(aCreationInfo)) {}
    ~Texture();

    bool IsLoadedToGPU() const { return loadedToGpu; }
    bool LoadToGpu(vk::Device aLogicalDevice,
                   vk::PhysicalDevice physicalDevice,
                   vk::Queue graphicsQueue,
                   vk::CommandPool graphicsCommandPool);

    vk::Image image;
    vk::ImageLayout imageLayout;
    vk::DeviceMemory deviceMemory;

    vk::DescriptorImageInfo descriptor;
    vk::Format format;
    vk::Sampler sampler;

    uint32_t width;
    uint32_t height;
    uint32_t mipLevels;
    uint32_t imageLayersCount;

    bool loadedToGpu = false;

   private:
    void Destroy();

    TextureCreationInfo creationInfo;
    vk::Device logicalDevice;
};

}  // namespace ez
