#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

#include "core/config.hpp"

namespace ez
{
const float kSmallFloat = 0.000000001f;

Camera::Camera()
{
    _fov = glm::radians(CameraConfig::fovDegrees);
    _near = CameraConfig::planeNear;
    _far = CameraConfig::planeFar;
    _position = CameraConfig::position;
    _targetPosition = CameraConfig::targetPosition;
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
    viewMatrix = glm::lookAt(_position, _targetPosition, CameraConfig::up);
    projectionMatrix = glm::perspective(_fov, aspectRatio, _near, _far);

    if (CameraConfig::invertProjection) { projectionMatrix[1][1] *= -1; }

    viewProjectionMatrix = projectionMatrix * viewMatrix;
}
}  // namespace ez
