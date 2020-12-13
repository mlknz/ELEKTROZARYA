#pragma once

#include "core/scene/scene.hpp"

namespace ez
{
class View
{
public:
    void SwitchToDefaultScene();

    std::shared_ptr<Scene> GetScene() const { return scene; }

private:
    std::shared_ptr<Scene> scene;
};
}
