#include "input.hpp"

#include "core/log_assert.hpp"

namespace ez
{
void Input::OnNewFrame()
{
    keyboardReleasedLastUpdate.clear();
    mouseRightPressedPrevPos = mouseRightPressedPos;
}

void Input::ProcessSDLEvent(SDL_Window* window, const SDL_Event& event)
{
    switch (event.type)
    {
        case SDL_KEYDOWN:
            keyboardPressed.insert(event.key.keysym.sym);
            break;
        case SDL_KEYUP:
            keyboardPressed.erase(event.key.keysym.sym);
            keyboardReleasedLastUpdate.insert(event.key.keysym.sym);
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_RIGHT || event.button.button == SDL_BUTTON_X1)
            {
                mouseRightPressedPrevPos = {};
                mouseRightPressedPos = {};
                mouseLeftStartX = event.button.x;
                mouseLeftStartY = event.button.y;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_RIGHT || event.button.button == SDL_BUTTON_X1)
            {
                mouseRightPressedPrevPos = {};
                mouseRightPressedPos = {};
                SDL_SetRelativeMouseMode(SDL_FALSE);
                SDL_WarpMouseInWindow(window, mouseLeftStartX, mouseLeftStartY);
            }
            break;
        case SDL_MOUSEMOTION:
            if (event.button.button == SDL_BUTTON_RIGHT || event.button.button == SDL_BUTTON_X1)
            {
                SDL_SetRelativeMouseMode(SDL_TRUE);
                mouseRightPressedPos = glm::vec2{ event.motion.x, event.motion.y };
            }
            break;
        default:
            break;
    }
}

bool Input::IsKeyPressed(SDL_Keycode key) const
{
    return keyboardPressed.find(key) != keyboardPressed.end();
}

bool Input::IsKeyReleasedLastUpdate(SDL_Keycode key) const
{
    return keyboardReleasedLastUpdate.find(key) != keyboardReleasedLastUpdate.end();
}

glm::vec2 Input::GetMouseDeltaLeftPressed(uint32_t viewportWidth, uint32_t viewportHeight) const
{
    if (!mouseRightPressedPos || !mouseRightPressedPrevPos || viewportWidth == 0 ||
        viewportHeight == 0)
    {
        return glm::vec2{ 0.0f, 0.0f };
    }

    glm::vec2 delta = *mouseRightPressedPos - *mouseRightPressedPrevPos;
    delta.x /= static_cast<float>(viewportWidth);
    delta.y /= static_cast<float>(viewportHeight);

    return delta;
}
}  // namespace ez
