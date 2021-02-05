#include "core/scene/scene.hpp"

namespace ez
{
bool Scene::Load()
{
    models.emplace_back("../assets/DamagedHelmet/glTF/DamagedHelmet.gltf");

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
