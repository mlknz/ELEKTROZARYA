#pragma once

#include "core/scene/scene.hpp"

namespace ez
{
class View
{
public:
    void SwitchToDefaultScene();
    void ToggleSceneTest();

    std::shared_ptr<Scene> GetScene() const { return scene; }

private:
    std::shared_ptr<Scene> scene;
};
}
