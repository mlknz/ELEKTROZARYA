#include <SDL.h>
#include <SDL_syswm.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "core/input/input.hpp"
#include "core/log_assert.hpp"
#include "core/scene/scene.hpp"
#include "core/view.hpp"
#include "gameplay/gameplay.hpp"
#include "render/render_system.hpp"

int main(int, char*[])
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    auto renderSystemRV = ez::RenderSystem::Create();
    if (renderSystemRV.result != ez::GraphicsResult::Ok)
    {
        EZLOG("Failed to create RenderSystem");
        return EXIT_FAILURE;
    }
    std::unique_ptr<ez::RenderSystem> renderSystem = std::move(renderSystemRV.value);

    auto gameplay = std::make_unique<ez::Gameplay>(std::make_unique<ez::View>());

    std::chrono::time_point prevTime = std::chrono::high_resolution_clock::now();
    int64_t deltaTimeMcs = -1.0;

    bool run = true;
    while (run)
    {
        gameplay->GetInput()->OnNewFrame();

        SDL_Event evt;
        while (SDL_PollEvent(&evt))
        {
            if (evt.type == SDL_QUIT) { run = false; }
            gameplay->GetInput()->ProcessSDLEvent(renderSystem->GetWindow(), evt);
        }
        uint32_t sdlWindowsFlags = SDL_GetWindowFlags(renderSystem->GetWindow());
        const bool isWindowMinimized =
            static_cast<bool>(sdlWindowsFlags & SDL_WINDOW_MINIMIZED);

        std::chrono::time_point currentTime = std::chrono::high_resolution_clock::now();

        deltaTimeMcs =
            std::chrono::duration_cast<std::chrono::microseconds>(currentTime - prevTime)
                .count();

        prevTime = currentTime;

        const std::unique_ptr<ez::View>& curView = gameplay->GetView();
        std::shared_ptr<ez::Scene> curScene = curView->GetScene();

        if (!curScene->IsLoaded())
        {
            bool sceneLoadSuccess = curScene->Load();
            EZASSERT(sceneLoadSuccess, "Failed to load Scene");
        }

        const vk::Extent2D& swapchainViewportExtent = renderSystem->GetViewportExtent();
        const bool drawThisFrame = !isWindowMinimized && swapchainViewportExtent.height != 0 &&
                                   swapchainViewportExtent.width != 0;

        if (drawThisFrame)
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL2_NewFrame(renderSystem->GetWindow());
            gameplay->SetViewportExtent(swapchainViewportExtent.width,
                                        swapchainViewportExtent.height);
        }

        gameplay->Update(deltaTimeMcs, drawThisFrame);

        if (drawThisFrame)
        {
            renderSystem->PrepareToRender(curScene);
            renderSystem->Draw(curView, gameplay->GetActiveCamera());
        }
    }

    gameplay.reset();
    renderSystem.reset();

    SDL_Quit();

    return EXIT_SUCCESS;
}
