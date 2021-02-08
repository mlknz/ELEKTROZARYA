#pragma once

#include <glm/gtc/type_ptr.hpp>
#include <string>

namespace ez
{
namespace CameraConfig
{
constexpr float fovDegrees = 60.0f;
constexpr float planeNear = 0.1f;
constexpr float planeFar = 40.0f;
constexpr glm::vec3 position{ 1.5f, 0.0f, 2.5f };

constexpr glm::vec3 targetPosition{ 0.0f, 0.0f, 0.0f };
constexpr glm::vec3 up{ 0.0f, 1.0f, 0.0f };

constexpr bool invertProjection = true;

}  // namespace CameraConfig

namespace SceneConfig
{
const std::string startupModel{ "../assets/DamagedHelmet/glTF/DamagedHelmet.gltf" };
}

}  // namespace ez
