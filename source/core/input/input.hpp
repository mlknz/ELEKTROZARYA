#pragma once

#include <SDL_events.h>
#include <SDL_keycode.h>

#include <glm/gtc/type_ptr.hpp>
#include <optional>
#include <set>

namespace ez
{
class Input
{
   public:
    void OnNewFrame();
    void ProcessSDLEvent(SDL_Window*, const SDL_Event&);

    bool IsKeyPressed(SDL_Keycode) const;
    bool IsKeyReleasedLastUpdate(SDL_Keycode) const;

    glm::vec2 GetMouseDeltaLeftPressed(uint32_t viewportWidth, uint32_t viewportHeight) const;

   private:
    std::set<SDL_Keycode> keyboardPressed;
    std::set<SDL_Keycode> keyboardReleasedLastUpdate;

    int32_t mouseLeftStartX = 0;
    int32_t mouseLeftStartY = 0;
    std::optional<glm::vec2> mouseLeftPressedPos;
    std::optional<glm::vec2> mouseLeftPressedPrevPos;
};
}  // namespace ez
