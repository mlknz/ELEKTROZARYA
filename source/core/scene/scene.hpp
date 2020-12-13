#pragma once

#include <vector>
#include "core/scene/mesh.hpp"

namespace ez {

class Scene final
{
public:
    bool Load();
    bool IsLoaded() const { return loaded; }

    void SetReadyToRender(bool value) { readyToRender = value; }
    bool ReadyToRender() const { return readyToRender; }

    const std::vector<Mesh>& GetMeshes() { return meshes; }

private:
    int sceneId = 0;
    std::vector<Mesh> meshes;

    bool loaded = false;
    bool readyToRender = false;
};

}
