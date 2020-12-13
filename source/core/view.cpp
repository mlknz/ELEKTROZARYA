#include "view.hpp"

namespace ez {

void View::SwitchToDefaultScene()
{
    scene = std::make_shared<Scene>();
}

}
