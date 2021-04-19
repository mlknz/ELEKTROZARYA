#pragma once

#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <vector>

#include "render/highlevel/material.hpp"
#include "render/highlevel/primitive.hpp"
#include "render/highlevel/texture.hpp"
#include "render/highlevel/texture_sampler.hpp"
#include "render/vulkan/vulkan_graphics_pipeline.hpp"
#include "render/vulkan_include.hpp"

namespace tinygltf
{
class Node;
class Model;
}  // namespace tinygltf

namespace ez
{
struct Mesh
{
    vk::Device device;

    std::vector<std::unique_ptr<Primitive>> primitives;

    BoundingBox bb;

    struct PushConstantsBlock final
    {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
    } pushConstantsBlock;
    static constexpr uint32_t PushConstantsBlockSize = sizeof(Mesh::PushConstantsBlock);

    Mesh(const glm::mat4& matrix) { this->pushConstantsBlock.modelMatrix = matrix; }

    void SetBoundingBox(glm::vec3 min, glm::vec3 max)
    {
        bb.min = min;
        bb.max = max;
        bb.valid = true;
    }
};

struct Node
{
    Node* parent = nullptr;

    std::string name;
    uint32_t index;
    std::unique_ptr<Mesh> mesh;
    std::vector<std::unique_ptr<Node>> children;

    glm::mat4 matrix{ 1.0f };
    glm::vec3 translation{ 0.0f };
    glm::vec3 scale{ 1.0f };
    glm::mat4 rotation{ 1.0f };
    BoundingBox aabb;

    inline glm::mat4 ConstructLocalMatrix()
    {
        glm::mat4 tr = glm::translate(glm::mat4(1.0f), translation);
        glm::mat4 sc = glm::scale(glm::mat4(1.0f), scale);
        return tr * rotation * sc * matrix;
    }

    inline glm::mat4 GetMatrix()
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
        if (mesh) { mesh->pushConstantsBlock.modelMatrix = GetMatrix(); }

        for (std::unique_ptr<Node>& child : children) { child->Update(); }
    }
};

struct Model
{
    enum class eType
    {
        GltfMesh,
        Cubemap
    };

    Model() = delete;
    Model(const Model& other) = delete;

    Model(eType type, const std::string& filePath);
    Model(Model&& other) = default;
    ~Model();

    void SetLogicalDevice(vk::Device device) { logicalDevice = device; }
    bool CreateVertexBuffers(vk::PhysicalDevice physicalDevice,
                             vk::Queue graphicsQueue,
                             vk::CommandPool graphicsCommandPool);
    VertexLayout GetVertexLayout() const { return vertexLayout; }

    std::string name;
    std::vector<std::unique_ptr<Node>> nodes;

    vk::Buffer vertexBuffer;
    vk::Buffer indexBuffer;

    std::shared_ptr<VulkanGraphicsPipeline> graphicsPipeline;

    std::vector<TextureSampler> textureSamplers;
    std::vector<Texture> textures;
    std::vector<Material> materials;

    // todo: move to Material
    std::string vertexShaderName;
    std::string fragmentShaderName;

   private:
    void LoadNodeFromGLTF(Node* parent,
                          const tinygltf::Node& node,
                          uint32_t nodeIndex,
                          const tinygltf::Model& model,
                          std::vector<uint32_t>& indexBuffer,
                          std::vector<Vertex>& vertexBuffer);

    void LoadMaterials(tinygltf::Model& model);

    VertexLayout vertexLayout = eVertexLayout::vlNone;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    vk::Device logicalDevice;

    vk::DeviceMemory vertexBufferMemory;
    vk::DeviceMemory indexBufferMemory;

    eType type;
};

namespace StbImageLoader
{
struct StbImageWrapper
{
    ~StbImageWrapper();

    int width;
    int height;
    int channelsCount;

    bool isHdr;

    float* hdrData;
};

StbImageWrapper LoadHDRImage(char const* path);
}  // namespace StbImageLoader

}  // namespace ez
