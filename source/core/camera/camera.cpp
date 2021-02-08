#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "core/config.hpp"

namespace ez
{
const float kSmallFloat = 0.000000001f;

Camera::Camera() { ResetToDefault(); }

void Camera::ResetToDefault()
{
    _fov = glm::radians(CameraConfig::fovDegrees);
    _near = CameraConfig::planeNear;
    _far = CameraConfig::planeFar;
    _position = CameraConfig::position;
    _targetPosition = CameraConfig::targetPosition;

    Recalc();
}

void Camera::SetViewportExtent(uint32_t width, uint32_t height)
{
    if (width == viewportWidth && height == viewportHeight) { return; }

    viewportWidth = width;
    viewportHeight = height;

    Recalc();
}

void Camera::MoveLocal(const glm::vec3& offsetLocal)
{
    if (glm::length(offsetLocal) < kSmallFloat) { return; }

    glm::mat4 curRotation = glm::inverse(viewMatrix);
    curRotation[3][0] = 0.0f;
    curRotation[3][1] = 0.0f;
    curRotation[3][2] = 0.0f;

    glm::vec3 offset = curRotation * glm::vec4(offsetLocal, 1.0f);

    _position += offset;
    _targetPosition += offset;

    Recalc();
}

void Camera::RotateLocal(const glm::vec2& anglesLocal)
{
    if (glm::length(anglesLocal) < kSmallFloat) { return; }

    glm::mat4 curRotation = glm::inverse(viewMatrix);
    curRotation[3][0] = 0.0f;
    curRotation[3][1] = 0.0f;
    curRotation[3][2] = 0.0f;

    glm::vec3 curUp = curRotation[1];
    glm::vec3 curRight = curRotation[0];
    glm::vec3 curForward = _targetPosition - _position;

    glm::vec3 rotatedX = glm::rotate(curForward, -anglesLocal.x, curUp);
    glm::vec3 rotatedXY = glm::rotate(rotatedX, -anglesLocal.y, curRight);

    _targetPosition = _position + rotatedXY;

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
