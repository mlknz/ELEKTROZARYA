#include "view.hpp"

#include "core/log_assert.hpp"
namespace ez
{
void View::SwitchToDefaultScene() { scene = std::make_shared<Scene>(); }

void View::ToggleSceneTest() { EZASSERT(false, "Not Implemented"); }

}  // namespace ez
