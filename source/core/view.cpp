#include "view.hpp"

namespace ez {

void View::SwitchToDefaultScene()
{
    activeScene = std::make_shared<Scene>();
}

}
