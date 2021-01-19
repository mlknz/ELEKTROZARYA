#include "view.hpp"

namespace ez
{
void View::SwitchToDefaultScene() { scene = std::make_shared<Scene>(); }

bool sceneChanged = false;

void View::ToggleSceneTest()
{
    if (!sceneChanged)
    {
        scene = std::make_shared<Scene>();
        scene->sceneId = 1;
        sceneChanged = true;
    }
}

}  // namespace ez
