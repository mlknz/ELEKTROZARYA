#pragma once

#include <vector>
#include "Mesh.hpp"

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
