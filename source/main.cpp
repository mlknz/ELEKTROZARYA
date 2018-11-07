#include <iostream>
#include <chrono>

#include <SDL2/SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Render/RenderSystem.hpp"

float timeAdd = 0.0f;

int main() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    auto renderSystem = std::make_unique<Ride::RenderSystem>();
    if (!renderSystem->Ready())
    {
        printf("Failed to create RenderSystem");
        return EXIT_FAILURE;
    }

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
            if (evt.type == SDL_KEYDOWN)
            {
                timeAdd += 0.25f;
            }
        }

        // update scene
        const Ride::SwapchainInfo& swapchainInfo = renderSystem->GetSwapchainInfo();
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;
        time += timeAdd;

        UniformBufferObject ubo = {};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), swapchainInfo.extent.width / (float) swapchainInfo.extent.height, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        renderSystem->UpdateUBO(ubo);

        // draw
        renderSystem->Draw();
    }

    renderSystem.reset();

    SDL_Quit();

    return EXIT_SUCCESS;
}
