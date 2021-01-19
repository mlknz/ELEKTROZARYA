#include "core/scene/scene.hpp"

namespace ez
{
bool Scene::Load()
{
    Mesh a = GetTestMesh(sceneId);
    meshes.push_back(std::move(a));

    readyToRender = false;
    loaded = true;

    return loaded;
}

}  // namespace ez
