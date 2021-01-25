#pragma once

#include <SDL_events.h>
#include <SDL_keycode.h>

#include <set>

namespace ez
{
class Input
{
   public:
    void ProcessSDLEvent(const SDL_Event&);

    bool IsKeyPressed(SDL_Keycode) const;
    bool IsKeyReleasedLastUpdate(SDL_Keycode) const;

   private:
    std::set<SDL_Keycode> keyboardPressed;
    std::set<SDL_Keycode> keyboardReleasedLastUpdate;
};
}  // namespace ez
