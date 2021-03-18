#include "scene.hpp"

#include "core/config.hpp"
#include "render/config.hpp"

namespace ez
{
bool Scene::Load()
{
    // models.emplace_back(Model::eType::Cubemap, SceneConfig::panorama);
    models.emplace_back(Model::eType::GltfMesh, SceneConfig::startupModel);

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
