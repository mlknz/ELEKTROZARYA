#include "gameplay.hpp"

#include <imgui/imgui.h>

#include <numeric>

#include "core/camera/camera.hpp"
#include "core/config.hpp"
#include "core/input/input.hpp"
#include "core/log_assert.hpp"
#include "core/view.hpp"
#include "render/config.hpp"

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

void Gameplay::Update(int64_t deltaTimeMcs, bool drawThisFrame)
{
    double deltaTimeSeconds = static_cast<double>(deltaTimeMcs) / (1000 * 1000);
    glm::vec3 cameraMovementLocal = glm::vec3(0.0f, 0.0f, 0.0f);
    if (input->IsKeyPressed(SDLK_a)) { cameraMovementLocal.x -= 1.0f; }
    if (input->IsKeyPressed(SDLK_d)) { cameraMovementLocal.x += 1.0f; }
    if (input->IsKeyPressed(SDLK_q)) { cameraMovementLocal.y -= 1.0f; }
    if (input->IsKeyPressed(SDLK_e)) { cameraMovementLocal.y += 1.0f; }
    if (input->IsKeyPressed(SDLK_s)) { cameraMovementLocal.z += 1.0f; }
    if (input->IsKeyPressed(SDLK_w)) { cameraMovementLocal.z -= 1.0f; }

    cameraMovementLocal *= deltaTimeSeconds;

    camera->MoveLocal(cameraMovementLocal * CameraConfig::moveSpeed);

    const glm::vec2& mouseDeltaPressed = input->GetMouseDeltaLeftPressed(
        camera->GetViewportWidth(), camera->GetViewportHeight());

    camera->RotateLocal(mouseDeltaPressed *
                        glm::vec2(CameraConfig::rotateXSpeed, CameraConfig::rotateYSpeed));

    if (input->IsKeyReleasedLastUpdate(SDLK_r)) { ReloadScene(); }
    if (input->IsKeyReleasedLastUpdate(SDLK_c)) { camera->ResetToDefault(); }

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

    if (drawThisFrame)
    {
        ImGui::NewFrame();
        ImGui::Begin("Control Panel");

        ImGui::Text(
            "Frame time (ms): %d.%d", int(avgFrameTime) / 1000, int(avgFrameTime) % 1000);
        ImGui::Text("FPS: %lf", 1.0 / avgDeltaTimeSeconds);
        if (ImGui::Button("Reload Scene")) { ReloadScene(); }
        if (ImGui::Button("Reset Camera")) { camera->ResetToDefault(); }
        // if (ImGui::Button("Toggle Scene Test")) { view->ToggleSceneTest(); }
        ImGui::Checkbox("MSAA 8x",
                        &Config::msaa8xEnabled);  // will be forced back if not supported

        // ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::End();
    }

    if (view && view->GetScene()) { view->GetScene()->Update(); }
}

void Gameplay::ReloadScene()
{
    if (view && view->GetScene()) { view->GetScene()->SetReadyToRender(false); }
}

}  // namespace ez
