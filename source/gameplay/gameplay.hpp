#pragma once

#include <memory>

namespace Ride {

class Camera;
class View;
class Input;

class Gameplay
{
public:
    Gameplay(std::unique_ptr<View>&& aView);

    std::unique_ptr<Input>& GetInput() { return input; }

    const std::unique_ptr<View>& GetView() const { return view; }
    const std::unique_ptr<Camera>& GetActiveCamera() const { return camera; }

    void SetViewportExtent(uint32_t width, uint32_t height);
    void Update(double curTime, double deltaTime);

private:
    std::unique_ptr<View> view;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Input> input;
};

}
