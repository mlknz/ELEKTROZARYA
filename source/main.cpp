#include <iostream>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui.h> // mewmew

#include <SDL.h>
#include <SDL_syswm.h>

#include "core/view.hpp"
#include "core/input/input.hpp"
#include "core/scene/scene.hpp"
#include "render/render_system.hpp"
#include "gameplay/gameplay.hpp"

int main(int, char* [])
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    auto renderSystemRV = ez::RenderSystem::Create();
    if (renderSystemRV.result != ez::GraphicsResult::Ok)
    {
        printf("Failed to create RenderSystem");
        return EXIT_FAILURE;
    }
    std::unique_ptr<ez::RenderSystem> renderSystem = std::move(renderSystemRV.value);

    auto gameplay = std::make_unique<ez::Gameplay>(std::make_unique<ez::View>());

    auto startTime = std::chrono::high_resolution_clock::now();
    double prevTime = -1.0;
    double curTime = -1.0;
    double deltaTime = -1.0;
    // ImGui::Text("Hello, %d", 42);
    bool run = true;
    while (run)
    {
        SDL_Event evt;
        while (SDL_PollEvent(&evt))
        {
            if (evt.type == SDL_QUIT)
            {
                run = false;
            }
            gameplay->GetInput()->ProcessSDLEvent(evt);
        }

        auto currentTime = std::chrono::high_resolution_clock::now();

        prevTime = curTime;
        curTime = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count()) / 1000.0;
        if (prevTime > 0.0 && curTime > 0.0)
        {
            deltaTime = curTime - prevTime;
        }

        const std::unique_ptr<ez::View>& curView = gameplay->GetView();
        std::shared_ptr<ez::Scene> curScene = curView->GetScene();

        if (!curScene->IsLoaded())
        {
            bool sceneLoadSuccess = curScene->Load(renderSystem->GetDevice());
            if (!sceneLoadSuccess)
            {
                printf("Failed to load Scene");
            }
        }

        const vk::Extent2D& viewportExtent = renderSystem->GetViewportExtent();
        gameplay->SetViewportExtent(viewportExtent.width, viewportExtent.height);
        gameplay->Update(curTime, deltaTime);

        renderSystem->PrepareToRender(curScene);

        renderSystem->Draw(curView, gameplay->GetActiveCamera());
    }

    renderSystem.reset();
    gameplay.reset();

    SDL_Quit();

    return EXIT_SUCCESS;
}
