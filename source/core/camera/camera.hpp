#pragma once

#include <glm/glm.hpp>

namespace ez
{
class Camera
{
   public:
    Camera();

    const glm::mat4& GetViewMatrix() const { return viewMatrix; }
    const glm::mat4& GetProjectionMatrix() const { return projectionMatrix; }
    const glm::mat4& GetViewProjectionMatrix() const { return viewProjectionMatrix; }
    uint32_t GetViewportWidth() const { return viewportWidth; }
    uint32_t GetViewportHeight() const { return viewportHeight; }

    void SetViewportExtent(uint32_t width, uint32_t height);

    void MoveLocal(const glm::vec3& offsetLocal);
    void RotateLocal(const glm::vec2& anglesLocal);
    void ResetToDefault();

   private:
    void Recalc();

    float _fov;
    float _near;
    float _far;
    glm::vec3 _position;
    glm::vec3 _targetPosition;

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;

    glm::mat4 viewProjectionMatrix;

    uint32_t viewportWidth;
    uint32_t viewportHeight;
};

}  // namespace ez
