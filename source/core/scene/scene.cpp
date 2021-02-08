#include "scene.hpp"

#include "core/config.hpp"

namespace ez
{
bool Scene::Load()
{
    models.emplace_back(SceneConfig::startupModel);

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
