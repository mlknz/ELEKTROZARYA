#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "render/highlevel/material.hpp"
#include "render/vulkan_include.hpp"

namespace tinygltf
{
class Node;
class Model;
}  // namespace tinygltf

namespace ez
{
using VertexLayout = uint16_t;
enum eVertexLayout : uint16_t
{
    vlNone = 0,
    vlPosition,
    vlNormal,
    vlTexcoord0,
    vlTexcoord1,
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv0;
    glm::vec2 uv1;

    static vk::VertexInputBindingDescription getBindingDescription()
    {
        vk::VertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;

        return bindingDescription;
    }

    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions(
        VertexLayout vertexLayout);
};

struct BoundingBox
{
    BoundingBox() {}
    BoundingBox(glm::vec3 min, glm::vec3 max) : min(min), max(max) {}
    BoundingBox GetAABB(glm::mat4 m);

    glm::vec3 min;
    glm::vec3 max;
    bool valid = false;
};

struct Primitive
{
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t vertexCount;
    bool hasIndices;

    BoundingBox bb;
    Material& material;

    Primitive(uint32_t firstIndex,
              uint32_t indexCount,
              uint32_t vertexCount,
              Material& material)
        : firstIndex(firstIndex)
        , indexCount(indexCount)
        , vertexCount(vertexCount)
        , material(material)
    {
        hasIndices = indexCount > 0;
    }

    void SetBoundingBox(glm::vec3 min, glm::vec3 max)
    {
        bb.min = min;
        bb.max = max;
        bb.valid = true;
    }
};

}  // namespace ez
