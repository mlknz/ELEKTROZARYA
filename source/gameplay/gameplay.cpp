#include "gameplay.hpp"

#include <imgui/imgui.h>

#include <numeric>

#include "core/camera/camera.hpp"
#include "core/input/input.hpp"
#include "core/log_assert.hpp"
#include "core/view.hpp"

namespace ez
{
Gameplay::Gameplay(std::unique_ptr<View>&& aView) : view(std::move(aView))
{
    view->SwitchToDefaultScene();

    camera = std::make_unique<Camera>();
    input = std::make_unique<Input>();
}

void Gameplay::SetViewportExtent(uint32_t width, uint32_t height)
{
    if (camera) { camera->SetViewportExtent(width, height); }
}

void Gameplay::Update(int64_t deltaTimeMcs)
{
    double deltaTimeSeconds = static_cast<double>(deltaTimeMcs) / (1000 * 1000);
    glm::vec3 cameraMovement;
    if (input->IsKeyPressed(SDLK_a)) { cameraMovement.x += 1.0f; }
    if (input->IsKeyPressed(SDLK_d)) { cameraMovement.x -= 1.0f; }
    if (input->IsKeyPressed(SDLK_s)) { cameraMovement.z -= 1.0f; }
    if (input->IsKeyPressed(SDLK_w)) { cameraMovement.z += 1.0f; }
    cameraMovement *= deltaTimeSeconds;

    camera->MovePreserveDirection(cameraMovement);

    int64_t frameTimesSum = 0;
    frameTimesMcs.emplace_front(deltaTimeMcs);
    auto it = frameTimesMcs.begin();
    for (; it != frameTimesMcs.end(); ++it)
    {
        frameTimesSum += *it;
        if (frameTimesSum > 100000) { break; }
    }
    if (it != frameTimesMcs.end()) { frameTimesMcs.erase(it, frameTimesMcs.end()); }

    const int64_t avgFrameTime =
        frameTimesMcs.empty() ? 0 : frameTimesSum / int64_t(frameTimesMcs.size());
    const double avgDeltaTimeSeconds = static_cast<double>(avgFrameTime) / (1000 * 1000);
    ImGui::NewFrame();
    ImGui::Begin("Control Panel");

    ImGui::Text("Frame time (ms): %d.%d", int(avgFrameTime) / 1000, int(avgFrameTime) % 1000);
    ImGui::Text("FPS: %lf", 1.0 / avgDeltaTimeSeconds);
    if (ImGui::Button("Toggle Scene Test")) { view->ToggleSceneTest(); }

    // ImGui::ShowDemoWindow();
    // ImGui::Text("Hello, %d", 42);
    // ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    ImGui::End();
}

}  // namespace ez
