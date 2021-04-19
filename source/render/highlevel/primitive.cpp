#include "primitive.hpp"

#include "core/log_assert.hpp"

namespace ez
{
BoundingBox BoundingBox::GetAABB(glm::mat4 m)
{
    glm::vec3 min = glm::vec3(m[3]);
    glm::vec3 max = min;
    glm::vec3 v0, v1;

    glm::vec3 right = glm::vec3(m[0]);
    v0 = right * this->min.x;
    v1 = right * this->max.x;
    min += glm::min(v0, v1);
    max += glm::max(v0, v1);

    glm::vec3 up = glm::vec3(m[1]);
    v0 = up * this->min.y;
    v1 = up * this->max.y;
    min += glm::min(v0, v1);
    max += glm::max(v0, v1);

    glm::vec3 back = glm::vec3(m[2]);
    v0 = back * this->min.z;
    v1 = back * this->max.z;
    min += glm::min(v0, v1);
    max += glm::max(v0, v1);

    return BoundingBox(min, max);
}

std::vector<vk::VertexInputAttributeDescription> Vertex::getAttributeDescriptions(
    VertexLayout vertexLayout)
{
    std::vector<vk::VertexInputAttributeDescription> attributeDescriptions = {};

    if (vertexLayout & eVertexLayout::vlPosition)
    {
        attributeDescriptions.emplace_back();
        attributeDescriptions.back().binding = 0;
        attributeDescriptions.back().location = 0;
        attributeDescriptions.back().format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions.back().offset = offsetof(Vertex, position);
    }

    if (vertexLayout & eVertexLayout::vlNormal)
    {
        attributeDescriptions.emplace_back();
        attributeDescriptions.back().binding = 0;
        attributeDescriptions.back().location = 1;
        attributeDescriptions.back().format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions.back().offset = offsetof(Vertex, normal);
    }

    if (vertexLayout & eVertexLayout::vlTexcoord0)
    {
        attributeDescriptions.emplace_back();
        attributeDescriptions.back().binding = 0;
        attributeDescriptions.back().location = 2;
        attributeDescriptions.back().format = vk::Format::eR32G32Sfloat;
        attributeDescriptions.back().offset = offsetof(Vertex, uv0);
    }

    if (vertexLayout & eVertexLayout::vlTexcoord1)
    {
        attributeDescriptions.emplace_back();
        attributeDescriptions.back().binding = 0;
        attributeDescriptions.back().location = 3;
        attributeDescriptions.back().format = vk::Format::eR32G32Sfloat;
        attributeDescriptions.back().offset = offsetof(Vertex, uv1);
    }

    return attributeDescriptions;
}
}  // namespace ez
