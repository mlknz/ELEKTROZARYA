#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Ride
{
Camera::Camera()
{
    _fov = glm::radians(45.0f);
    _near = 0.1f;
    _far = 10.0f;
}

void Camera::SetViewportExtent(uint32_t width, uint32_t height)
{
    if (width == viewportWidth && height == viewportHeight)
    {
        return;
    }

    viewportWidth = width;
    viewportHeight = height;

    float aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    viewMatrix = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    projectionMatrix = glm::perspective(_fov, aspectRatio, _near, _far);
    projectionMatrix[1][1] *= -1;

    viewProjectionMatrix = projectionMatrix * viewMatrix;
}
}
