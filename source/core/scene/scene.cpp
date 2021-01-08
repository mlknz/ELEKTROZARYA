#include "core/scene/scene.hpp"

namespace ez {

bool Scene::Load(vk::Device logicalDevice)
{
    meshes = { GetTestMesh(logicalDevice, sceneId) };

    readyToRender = false;
    loaded = true;

    return loaded;
}

}
