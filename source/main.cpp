#include <iostream>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SDL.h>
#include <SDL_syswm.h>

#include "core/view.hpp"
#include "core/input/input.hpp"
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

    auto view = std::make_unique<ez::View>();
    auto gameplay = std::make_unique<ez::Gameplay>(std::move(view));

    std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();
    double prevTime = -1.0;
    double curTime = -1.0;
    double deltaTime = -1.0;

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

        std::chrono::time_point currentTime = std::chrono::high_resolution_clock::now();

        prevTime = curTime;
        curTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0;
        if (prevTime > 0.0 && curTime > 0.0)
        {
            deltaTime = curTime - prevTime;
        }

        const vk::Extent2D& viewportExtent = renderSystem->GetViewportExtent();

        gameplay->SetViewportExtent(viewportExtent.width, viewportExtent.height);
        gameplay->Update(curTime, deltaTime);

        renderSystem->Draw(gameplay->GetView(), gameplay->GetActiveCamera());
    }

    renderSystem.reset();
    view.reset();
    gameplay.reset();

    SDL_Quit();

    return EXIT_SUCCESS;
}
