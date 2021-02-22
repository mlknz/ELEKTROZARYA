#include "scene.hpp"

#include "core/config.hpp"

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

    // mewmew todo:
    // 1. Generate env cubemap with computes (with mips)
    // 2. Draw cubemap on scene

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
