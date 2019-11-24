#pragma once

#include <set>
#include <SDL_events.h>
#include <SDL_keycode.h>

namespace Ride
{

class Input
{
public:
    void ProcessSDLEvent(const SDL_Event&);

    bool IsKeyPressed(SDL_Keycode) const;
private:
    std::set<SDL_Keycode> keyboardPressed;
};
}
