#include <iostream>
#include <chrono>

#include <SDL.h>
#include <SDL_syswm.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Render/RenderSystem.hpp"
#include "Scene/Scene.hpp"

int main() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    std::unique_ptr<Ride::RenderSystem> renderSystem = Ride::RenderSystem::Create();
    if (!renderSystem)
    {
        printf("Failed to create RenderSystem");
        return EXIT_FAILURE;
    }

    auto scene = std::make_shared<Ride::Scene>();

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
                printf("Keydown\n");
            }
        }

        const float screenWidth = static_cast<float>(renderSystem->GetScreenWidth());
        const float screenHeight = static_cast<float>(renderSystem->GetScreenHeight());

        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

        UniformBufferObject ubo = {};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), screenWidth / screenHeight, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        renderSystem->UpdateUBO(ubo);
        renderSystem->Draw(scene);
    }

    renderSystem.reset();

    SDL_Quit();

    return EXIT_SUCCESS;
}
