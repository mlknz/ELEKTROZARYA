#pragma once

#include <vector>

#include "core/scene/mesh.hpp"

namespace ez
{
class Scene final
{
   public:
    bool Load();
    bool IsLoaded() const { return loaded; }

    void SetReadyToRender(bool value) { readyToRender = value; }
    bool ReadyToRender() const { return readyToRender; }

    const std::vector<Model>& GetModels() { return models; }
    std::vector<Model>& GetModelsMutable() { return models; }

    int sceneId = 0;

   private:
    std::vector<Model> models;

    bool loaded = false;
    bool readyToRender = false;
};

}  // namespace ez
