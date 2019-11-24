#include "gameplay.hpp"

#include <core/view.hpp>
#include <core/camera/camera.hpp>

namespace Ride {

Gameplay::Gameplay(std::unique_ptr<View>&& aView) : view(std::move(aView))
{
    view->SwitchToDefaultScene();

    camera = std::make_unique<Camera>();
}

void Gameplay::SetViewportExtent(uint32_t width, uint32_t height)
{
    if (camera)
    {
        camera->SetViewportExtent(width, height);
    }
}

void Gameplay::Update(double curTime, double deltaTime)
{

}

}
