#pragma once

#include "source/Scene/Mesh.hpp"
#include <vector>

namespace Ride {

class Scene
{
public:
    Scene() {
        meshes = {GetTestMesh()};
    }
    int sceneId = 0;
    std::vector<Mesh> meshes;

};

}
