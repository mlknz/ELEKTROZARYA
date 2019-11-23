#include "view.hpp"

namespace Ride {

void View::SwitchToDefaultScene()
{
    activeScene = std::make_shared<Scene>();
}

}
