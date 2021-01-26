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

}  // namespace ez
