#pragma once

#include <vector>

#include "render/highlevel/texture_sampler.hpp"
#include "render/vulkan_include.hpp"

namespace ez
{
struct TextureCreationInfo final
{
    static TextureCreationInfo CreateFromData(unsigned char* data,
                                              uint32_t width,
                                              uint32_t height,
                                              uint32_t channelsCount,
                                              const TextureSampler& textureSampler);

    bool IsValid() const;

    // CPU data to load to GPU
    std::vector<unsigned char> buffer;
    vk::DeviceSize bufferSize = 0;
    TextureSampler textureSampler;

    // other
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 0;
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

    vk::ImageView imageView;
    vk::Image image;
    vk::ImageLayout imageLayout;
    vk::DeviceMemory deviceMemory;

    vk::DescriptorImageInfo descriptor;
    vk::Sampler sampler;

    uint32_t width;
    uint32_t height;
    uint32_t mipLevels;
    uint32_t layerCount;

    bool loadedToGpu = false;

   private:
    void Destroy();

    TextureCreationInfo creationInfo;
    vk::Device logicalDevice;
};

}  // namespace ez
