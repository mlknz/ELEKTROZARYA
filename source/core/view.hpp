#pragma once

#include "core/scene/scene.hpp"

namespace Ride
{
class View
{
public:
    void SwitchToDefaultScene();

    std::shared_ptr<Scene> GetActiveScene() const { return activeScene; }

private:
    std::shared_ptr<Scene> activeScene;
};
}
