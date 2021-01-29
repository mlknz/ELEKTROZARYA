#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtx/norm.hpp>

namespace ez
{
const float kSmallFloat = 0.000000001f;

Camera::Camera()
{
    _fov = glm::radians(45.0f);
    _near = 0.1f;
    _far = 10.0f;
    _position = glm::vec3(0.0f, 4.0f, 3.0f);
    _targetPosition = glm::vec3(0.0f, 0.0f, 0.0f);
}

void Camera::SetViewportExtent(uint32_t width, uint32_t height)
{
    if (width == viewportWidth && height == viewportHeight) { return; }

    viewportWidth = width;
    viewportHeight = height;

    Recalc();
}

void Camera::MovePreserveDirection(const glm::vec3& offset)
{
    if (glm::length(offset) < kSmallFloat) { return; }
    _position += offset;
    _targetPosition += offset;

    Recalc();
}

void Camera::Recalc()
{
    float aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    viewMatrix = glm::lookAt(_position, _targetPosition, glm::vec3(0.0f, 0.0f, 1.0f));
    projectionMatrix = glm::perspective(_fov, aspectRatio, _near, _far);
    projectionMatrix[1][1] *= -1;

    viewProjectionMatrix = projectionMatrix * viewMatrix;
}
}  // namespace ez
