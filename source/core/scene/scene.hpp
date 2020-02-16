#pragma once

#include <vector>
#include "core/scene/mesh.hpp"

namespace ez {

class Scene
{
public:
    Scene() {
        meshes = { GetTestMesh() };
    }
    int sceneId = 0;
    std::vector<Mesh> meshes;
};

}
