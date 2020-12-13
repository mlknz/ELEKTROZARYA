#include "core/scene/scene.hpp"

namespace ez {

bool Scene::Load()
{
    meshes = { GetTestMesh() };

    readyToRender = false;
    loaded = true;

    return loaded;
}

}
