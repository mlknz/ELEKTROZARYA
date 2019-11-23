#pragma once

#include <memory>

namespace Ride {

class Camera;
class View;

class Gameplay
{
public:
    Gameplay(std::unique_ptr<View>&& aView);
    ~Gameplay();
    void Update(double curTime, double deltaTime);

    const std::unique_ptr<View>& GetView() const { return view; }
    const std::unique_ptr<Camera>& GetActiveCamera() const { return camera; }
private:
    std::unique_ptr<View> view;
    std::unique_ptr<Camera> camera;
};

}
