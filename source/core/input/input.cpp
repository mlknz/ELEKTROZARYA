#include "input.hpp"

#include <iostream>

namespace ez
{
    void Input::ProcessSDLEvent(const SDL_Event& event)
    {
        switch(event.type)
        {
        case SDL_KEYDOWN:
            keyboardPressed.insert(event.key.keysym.sym);
            break;
        case SDL_KEYUP:
            keyboardPressed.erase(event.key.keysym.sym);
            break;
        default:
            break;
        }
    }

    bool Input::IsKeyPressed(SDL_Keycode key) const
    {
        return keyboardPressed.find(key) != keyboardPressed.end();
    }
}
