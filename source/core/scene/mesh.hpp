#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vector>

#include "render/vulkan_include.hpp"

namespace ez
{
struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription getBindingDescription()
    {
        vk::VertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;

        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
        std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions = {};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

struct Mesh
{
    Mesh() = default;
    ~Mesh();

    void SetLogicalDevice(vk::Device device) { logicalDevice = device; }
    bool CreateVertexBuffers(vk::PhysicalDevice physicalDevice,
                             vk::Queue graphicsQueue,
                             vk::CommandPool graphicsCommandPool);
    bool CreateDescriptorSet(vk::DescriptorPool descriptorPool,
                             vk::DescriptorSetLayout descriptorSetLayout,
                             size_t hardcodedGlobalUBOSize);

    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

    vk::Device logicalDevice;

    vk::Buffer vertexBuffer;
    vk::Buffer indexBuffer;
    vk::Buffer uniformBuffer;

    vk::DeviceMemory vertexBufferMemory;
    vk::DeviceMemory indexBufferMemory;
    vk::DeviceMemory uniformBufferMemory;

    uint32_t uniformBufferMaxHackSize = 200;  // todo: calc needed size

    vk::DescriptorSet descriptorSet;
};

Mesh GetTestMesh(int sceneIndex);
}  // namespace ez
