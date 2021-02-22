#pragma once

#include <memory>
#include <vector>

#include "render/highlevel/mesh.hpp"
#include "render/highlevel/texture.hpp"

namespace ez
{
class Scene final
{
   public:
    bool Load();
    bool IsLoaded() const { return loaded; }

    void SetReadyToRender(bool value) { readyToRender = value; }
    bool ReadyToRender() const { return readyToRender; }

    void Update();

    const std::vector<Model>& GetModels() { return models; }
    std::vector<Model>& GetModelsMutable() { return models; }

    Texture* GetPanoramaHdrTexture() { return panoramaHdrTexture.get(); }

    int sceneId = 0;

   private:
    std::vector<Model> models;
    std::unique_ptr<Texture> panoramaHdrTexture;

    bool loaded = false;
    bool readyToRender = false;
};

}  // namespace ez
