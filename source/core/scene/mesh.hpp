#pragma once

#include <array>
//#include <gli/gli.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <vector>

#include "core/scene/material.hpp"
#include "render/vulkan/vulkan_graphics_pipeline.hpp"
#include "render/vulkan_include.hpp"

namespace tinygltf
{
class Node;
class Model;
}  // namespace tinygltf

namespace ez
{
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

    static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions()
    {
        std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions = {};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[2].offset = offsetof(Vertex, uv0);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[3].offset = offsetof(Vertex, uv1);

        return attributeDescriptions;
    }
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

    void setBoundingBox(glm::vec3 min, glm::vec3 max)
    {
        bb.min = min;
        bb.max = max;
        bb.valid = true;
    }
};

struct Mesh
{
    vk::Device device;

    std::vector<std::unique_ptr<Primitive>> primitives;

    BoundingBox bb;
    BoundingBox aabb;

    struct UniformBuffer
    {
        vk::Buffer buffer;
        vk::DeviceMemory memory;
        vk::DescriptorBufferInfo descriptor;
        vk::DescriptorSet descriptorSet;
        void* mapped;
    } uniformBuffer;

    struct UniformBlock
    {
        glm::mat4 matrix;
        // glm::mat4 jointMatrix[MAX_NUM_JOINTS]{};
        // float jointcount{ 0 };
    } uniformBlock;

    Mesh(/*vk::Device device,*/ glm::mat4 matrix)
    {
        // this->device = device;
        this->uniformBlock.matrix = matrix;
        //        VK_CHECK_RESULT(device->createBuffer(
        //            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        //            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        //            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uniformBlock),
        //            &uniformBuffer.buffer,
        //            &uniformBuffer.memory,
        //            &uniformBlock));
        //        VK_CHECK_RESULT(vkMapMemory(device->logicalDevice,
        //                                    uniformBuffer.memory,
        //                                    0,
        //                                    sizeof(uniformBlock),
        //                                    0,
        //                                    &uniformBuffer.mapped));
        //        uniformBuffer.descriptor = { uniformBuffer.buffer, 0, sizeof(uniformBlock) };
    };

    ~Mesh()
    {
        //        vkDestroyBuffer(device->logicalDevice, uniformBuffer.buffer, nullptr);
        //        vkFreeMemory(device->logicalDevice, uniformBuffer.memory, nullptr);
        // for (Primitive* p : primitives) delete p;
    }

    void SetBoundingBox(glm::vec3 min, glm::vec3 max)
    {
        bb.min = min;
        bb.max = max;
        bb.valid = true;
    }
};

struct Node
{
    Node* parent;
    uint32_t index;
    std::vector<std::unique_ptr<Node>> children;
    glm::mat4 matrix;
    std::string name;
    std::unique_ptr<Mesh> mesh;
    glm::vec3 translation{};
    glm::vec3 scale{ 1.0f };
    glm::quat rotation{};
    BoundingBox aabb;

    glm::mat4 ConstructLocalMatrix()
    {
        return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) *
               glm::scale(glm::mat4(1.0f), scale) * matrix;
    }

    glm::mat4 GetMatrix()
    {
        glm::mat4 m = ConstructLocalMatrix();
        Node* p = parent;
        while (p)
        {
            m = p->ConstructLocalMatrix() * m;
            p = p->parent;
        }
        return m;
    }

    void Update()
    {
        /*
        if (mesh)
        {
            glm::mat4 m = GetMatrix();
            memcpy(mesh->uniformBuffer.mapped, &m, sizeof(glm::mat4));
        }

        for (auto& child : children) { child->Update(); }
        */
    }

    ~Node()
    {
        // if (subMesh) { delete subMesh; }
    }
};

struct Model
{
    Model() = delete;
    Model(const std::string& gltfFilePath);
    ~Model();
    Model(const Model& other) = delete;
    Model(Model&& other) = default;

    void SetLogicalDevice(vk::Device device) { logicalDevice = device; }
    bool CreateVertexBuffers(vk::PhysicalDevice physicalDevice,
                             vk::Queue graphicsQueue,
                             vk::CommandPool graphicsCommandPool);
    bool CreateDescriptorSet(vk::DescriptorPool descriptorPool,
                             vk::DescriptorSetLayout aDescriptorSetLayout,
                             size_t hardcodedGlobalUBOSize);

    vk::DescriptorSetLayout GetDescriptorSetLayout() const { return descriptorSetLayout; }

    std::string name;
    std::vector<std::unique_ptr<Node>> nodes;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    vk::Device logicalDevice;

    vk::Buffer vertexBuffer;
    vk::Buffer indexBuffer;
    vk::Buffer uniformBuffer;

    vk::DeviceMemory vertexBufferMemory;
    vk::DeviceMemory indexBufferMemory;
    vk::DeviceMemory uniformBufferMemory;

    uint32_t uniformBufferMaxHackSize = 192;

    vk::DescriptorSetLayout descriptorSetLayout;
    vk::DescriptorSet descriptorSet;
    std::shared_ptr<VulkanGraphicsPipeline> graphicsPipeline;

   private:
    void LoadNodeFromGLTF(Node* parent,
                          const tinygltf::Node& node,
                          uint32_t nodeIndex,
                          const tinygltf::Model& model,
                          std::vector<uint32_t>& indexBuffer,
                          std::vector<Vertex>& vertexBuffer);
};

}  // namespace ez
