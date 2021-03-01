#include "scene.hpp"

#include "core/config.hpp"
#include "render/config.hpp"

namespace ez
{
bool Scene::Load()
{
    models.emplace_back(SceneConfig::startupModel);

    StbImageLoader::StbImageWrapper hdrPanoramaWrapper =
        StbImageLoader::LoadHDRImage(SceneConfig::panorama.c_str());
    TextureSampler panoramaTexSampler = {};
    TextureCreationInfo panoramaTexCI = TextureCreationInfo::CreateHdrFromData(
        hdrPanoramaWrapper.hdrData,
        static_cast<uint32_t>(hdrPanoramaWrapper.width),
        static_cast<uint32_t>(hdrPanoramaWrapper.height),
        static_cast<uint32_t>(hdrPanoramaWrapper.channelsCount),
        panoramaTexSampler);
    panoramaHdrTexture = std::make_unique<Texture>(std::move(panoramaTexCI));

    uint32_t cubemapWidth = 512;
    uint32_t cubemapHeight = 512;
    const uint32_t cubemapColorChannelsCount = 4;
    uint32_t singleFaceSize = cubemapWidth * cubemapWidth * cubemapColorChannelsCount;

    const uint32_t cubemapImageLayersCount = 6;

    std::array<std::array<uint8_t, cubemapColorChannelsCount>, cubemapImageLayersCount>
        faceColors;
    faceColors[0] = { 255, 0, 0, 255 };
    faceColors[1] = { 128, 0, 0, 255 };
    faceColors[2] = { 0, 255, 0, 255 };
    faceColors[3] = { 0, 128, 0, 255 };
    faceColors[4] = { 0, 0, 255, 255 };
    faceColors[5] = { 0, 0, 128, 255 };

    std::vector<uint8_t> cubemapData;
    cubemapData.resize(singleFaceSize * cubemapImageLayersCount);
    for (uint32_t face = 0; face < cubemapImageLayersCount; ++face)
    {
        for (uint32_t i = 0; i < singleFaceSize; i += cubemapColorChannelsCount)
        {
            cubemapData[face * singleFaceSize + i + 0] = faceColors[face][0];
            cubemapData[face * singleFaceSize + i + 1] = faceColors[face][1];
            cubemapData[face * singleFaceSize + i + 2] = faceColors[face][2];
            cubemapData[face * singleFaceSize + i + 3] = faceColors[face][3];
        }
    }

    TextureSampler cubemapTexSampler = {};
    TextureCreationInfo cubemapTexCI =
        TextureCreationInfo::CreateFromData(cubemapData.data(),
                                            cubemapWidth,
                                            cubemapHeight,
                                            cubemapColorChannelsCount,
                                            cubemapImageLayersCount,
                                            true,
                                            cubemapTexSampler);
    cubemapTexCI.SetIsCubemap(true);
    cubemapTexture = std::make_unique<Texture>(std::move(cubemapTexCI));

    readyToRender = false;
    loaded = true;

    return loaded;
}

void Scene::Update()
{
    if (!IsLoaded()) { return; }
    for (Model& model : models)
    {
        for (std::unique_ptr<Node>& node : model.nodes) { node->Update(); }
    }
}

}  // namespace ez
