#include "mesh.hpp"

#include "core/log_assert.hpp"
#include "render/graphics_result.hpp"
#include "render/vulkan/vulkan_buffer.hpp"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

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

Model::Model(const std::string& gltfFilePath)
{
    EZLOG("loading gltf file", gltfFilePath);
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool fileLoaded = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, gltfFilePath);

    EZASSERT(fileLoaded, "Failed to load file");

    name = gltfFilePath;
    indices = {};
    vertices = {};

    const tinygltf::Scene& scene = gltfModel.scenes.at(size_t(gltfModel.defaultScene));
    for (size_t i = 0; i < scene.nodes.size(); i++)
    {
        const tinygltf::Node node = gltfModel.nodes.at(size_t(scene.nodes[i]));
        EZLOG("Loading Node", node.name);
        LoadNodeFromGLTF(nullptr, node, uint32_t(scene.nodes[i]), gltfModel, indices, vertices);
    }
}

void Model::LoadNodeFromGLTF(Node* parent,
                             const tinygltf::Node& node,
                             uint32_t nodeIndex,
                             const tinygltf::Model& model,
                             std::vector<uint32_t>& indexBuffer,
                             std::vector<Vertex>& vertexBuffer)
{
    std::unique_ptr<Node> newNode = std::make_unique<Node>();
    newNode->index = nodeIndex;
    newNode->parent = parent;
    newNode->name = node.name;
    newNode->matrix = glm::mat4(1.0f);

    // Generate local node matrix
    glm::vec3 translation = glm::vec3(0.0f);
    if (node.translation.size() == 3)
    {
        translation = glm::make_vec3(node.translation.data());
        newNode->translation = translation;
    }
    glm::mat4 rotation = glm::mat4(1.0f);
    if (node.rotation.size() == 4)
    {
        glm::quat q = glm::make_quat(node.rotation.data());
        newNode->rotation = glm::mat4(q);
    }
    glm::vec3 scale = glm::vec3(1.0f);
    if (node.scale.size() == 3)
    {
        scale = glm::make_vec3(node.scale.data());
        newNode->scale = scale;
    }
    if (node.matrix.size() == 16) { newNode->matrix = glm::make_mat4x4(node.matrix.data()); }

    // Node with children
    if (node.children.size() > 0)
    {
        for (size_t i = 0; i < node.children.size(); i++)
        {
            LoadNodeFromGLTF(newNode.get(),
                             model.nodes[node.children[i]],
                             node.children[i],
                             model,
                             indexBuffer,
                             vertexBuffer);
        }
    }

    // Node contains mesh data
    if (node.mesh > -1)
    {
        const tinygltf::Mesh mesh = model.meshes[node.mesh];
        newNode->mesh = std::make_unique<Mesh>(newNode->matrix);
        for (size_t j = 0; j < mesh.primitives.size(); j++)
        {
            const tinygltf::Primitive& primitive = mesh.primitives[j];
            uint32_t indexStart = static_cast<uint32_t>(indexBuffer.size());
            uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
            uint32_t indexCount = 0;
            uint32_t vertexCount = 0;
            glm::vec3 posMin{};
            glm::vec3 posMax{};
            bool hasSkin = false;
            bool hasIndices = primitive.indices > -1;
            // Vertices
            {
                const float* bufferPos = nullptr;
                const float* bufferNormals = nullptr;
                const float* bufferTexCoordSet0 = nullptr;
                const float* bufferTexCoordSet1 = nullptr;

                int posByteStride;
                int normByteStride;
                int uv0ByteStride;
                int uv1ByteStride;

                // Position attribute is required
                assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

                const tinygltf::Accessor& posAccessor =
                    model.accessors[primitive.attributes.find("POSITION")->second];
                const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
                bufferPos = reinterpret_cast<const float*>(
                    &(model.buffers[posView.buffer]
                          .data[posAccessor.byteOffset + posView.byteOffset]));
                posMin = glm::vec3(posAccessor.minValues[0],
                                   posAccessor.minValues[1],
                                   posAccessor.minValues[2]);
                posMax = glm::vec3(posAccessor.maxValues[0],
                                   posAccessor.maxValues[1],
                                   posAccessor.maxValues[2]);
                vertexCount = static_cast<uint32_t>(posAccessor.count);
                posByteStride =
                    posAccessor.ByteStride(posView)
                        ? (posAccessor.ByteStride(posView) / sizeof(float))
                        : (tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3) *
                           tinygltf::GetComponentSizeInBytes(TINYGLTF_COMPONENT_TYPE_FLOAT));

                if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
                {
                    const tinygltf::Accessor& normAccessor =
                        model.accessors[primitive.attributes.find("NORMAL")->second];
                    const tinygltf::BufferView& normView =
                        model.bufferViews[normAccessor.bufferView];
                    bufferNormals = reinterpret_cast<const float*>(
                        &(model.buffers[normView.buffer]
                              .data[normAccessor.byteOffset + normView.byteOffset]));
                    normByteStride = normAccessor.ByteStride(normView)
                                       ? (normAccessor.ByteStride(normView) / sizeof(float))
                                       : (tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3) *
                                          tinygltf::GetComponentSizeInBytes(
                                              TINYGLTF_COMPONENT_TYPE_FLOAT));
                }

                if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
                {
                    const tinygltf::Accessor& uvAccessor =
                        model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView& uvView =
                        model.bufferViews[uvAccessor.bufferView];
                    bufferTexCoordSet0 = reinterpret_cast<const float*>(
                        &(model.buffers[uvView.buffer]
                              .data[uvAccessor.byteOffset + uvView.byteOffset]));
                    uv0ByteStride = uvAccessor.ByteStride(uvView)
                                      ? (uvAccessor.ByteStride(uvView) / sizeof(float))
                                      : (tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2) *
                                         tinygltf::GetComponentSizeInBytes(
                                             TINYGLTF_COMPONENT_TYPE_FLOAT));
                }
                if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end())
                {
                    const tinygltf::Accessor& uvAccessor =
                        model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
                    const tinygltf::BufferView& uvView =
                        model.bufferViews[uvAccessor.bufferView];
                    bufferTexCoordSet1 = reinterpret_cast<const float*>(
                        &(model.buffers[uvView.buffer]
                              .data[uvAccessor.byteOffset + uvView.byteOffset]));
                    uv1ByteStride = uvAccessor.ByteStride(uvView)
                                      ? (uvAccessor.ByteStride(uvView) / sizeof(float))
                                      : (tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2) *
                                         tinygltf::GetComponentSizeInBytes(
                                             TINYGLTF_COMPONENT_TYPE_FLOAT));
                }

                for (size_t v = 0; v < posAccessor.count; v++)
                {
                    Vertex vert{};
                    vert.position =
                        glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]), 1.0f);
                    vert.normal = glm::normalize(glm::vec3(
                        bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride])
                                      : glm::vec3(0.0f)));
                    vert.uv0 = bufferTexCoordSet0
                                 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride])
                                 : glm::vec3(0.0f);
                    vert.uv1 = bufferTexCoordSet1
                                 ? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride])
                                 : glm::vec3(0.0f);

                    vertexBuffer.push_back(vert);
                }
            }
            // Indices
            if (hasIndices)
            {
                const tinygltf::Accessor& accessor =
                    model.accessors[primitive.indices > -1 ? primitive.indices : 0];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                indexCount = static_cast<uint32_t>(accessor.count);
                const void* dataPtr =
                    &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

                switch (accessor.componentType)
                {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                    {
                        const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                    {
                        const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                    {
                        const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    default:
                        std::cerr << "Index component type " << accessor.componentType
                                  << " not supported!" << std::endl;
                        return;
                }
            }
            // Material& mat = primitive.material > -1 ? materials[primitive.material] :
            // materials.back();
            Material mat;
            std::unique_ptr<Primitive> newPrimitive =
                std::make_unique<Primitive>(indexStart, indexCount, vertexCount, mat);
            newPrimitive->setBoundingBox(posMin, posMax);
            newNode->mesh->primitives.push_back(std::move(newPrimitive));
        }
        // Mesh BB from BBs of primitives
        for (auto& p : newNode->mesh->primitives)
        {
            if (p->bb.valid && !newNode->mesh->bb.valid)
            {
                newNode->mesh->bb = p->bb;
                newNode->mesh->bb.valid = true;
            }
            newNode->mesh->bb.min = glm::min(newNode->mesh->bb.min, p->bb.min);
            newNode->mesh->bb.max = glm::max(newNode->mesh->bb.max, p->bb.max);
        }
    }
    if (parent) { parent->children.push_back(std::move(newNode)); }
    else
    {
        nodes.push_back(std::move(newNode));
    }
}

Model::~Model()
{
    if (logicalDevice)
    {
        logicalDevice.destroyBuffer(uniformBuffer);
        logicalDevice.freeMemory(uniformBufferMemory);

        logicalDevice.destroyBuffer(indexBuffer);
        logicalDevice.freeMemory(indexBufferMemory);

        logicalDevice.destroyBuffer(vertexBuffer);
        logicalDevice.freeMemory(vertexBufferMemory);
    }
}

bool Model::CreateVertexBuffers(vk::PhysicalDevice physicalDevice,
                                vk::Queue graphicsQueue,
                                vk::CommandPool graphicsCommandPool)
{
    assert(logicalDevice);
    vk::DeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
    vk::DeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();

    VulkanBuffer::createBuffer(
        logicalDevice,
        physicalDevice,
        vertexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vertexBuffer,
        vertexBufferMemory);
    VulkanBuffer::createBuffer(
        logicalDevice,
        physicalDevice,
        indexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        indexBuffer,
        indexBufferMemory);
    VulkanBuffer::createBuffer(logicalDevice,
                               physicalDevice,
                               uniformBufferMaxHackSize,
                               vk::BufferUsageFlagBits::eUniformBuffer,
                               vk::MemoryPropertyFlagBits::eHostVisible |
                                   vk::MemoryPropertyFlagBits::eHostCoherent |
                                   vk::MemoryPropertyFlagBits::eDeviceLocal,
                               uniformBuffer,
                               uniformBufferMemory);

    vk::DebugUtilsObjectNameInfoEXT nameInfo;
    nameInfo.objectType = vk::ObjectType::eDeviceMemory;
    nameInfo.setPObjectName("MODEL_GLOBAL_UBO_MEMORY");
    const uint64_t objectHandle =
        reinterpret_cast<uint64_t>(uniformBufferMemory.operator VkDeviceMemory());
    nameInfo.objectHandle = objectHandle;

    CheckVkResult(logicalDevice.setDebugUtilsObjectNameEXT(nameInfo));

    VulkanBuffer::uploadData(logicalDevice,
                             physicalDevice,
                             graphicsQueue,
                             graphicsCommandPool,
                             vertexBuffer,
                             vertexBufferSize,
                             vertices.data());
    VulkanBuffer::uploadData(logicalDevice,
                             physicalDevice,
                             graphicsQueue,
                             graphicsCommandPool,
                             indexBuffer,
                             indexBufferSize,
                             indices.data());

    return true;
}

bool Model::CreateDescriptorSet(vk::DescriptorPool descriptorPool,
                                vk::DescriptorSetLayout aDescriptorSetLayout,
                                size_t hardcodedGlobalUBOSize)
{
    EZASSERT(logicalDevice);
    descriptorSetLayout = aDescriptorSetLayout;

    vk::DescriptorSetLayout layouts[] = { descriptorSetLayout };
    vk::DescriptorSetAllocateInfo allocInfo = {};
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;

    vk::Result allocResult = logicalDevice.allocateDescriptorSets(&allocInfo, &descriptorSet);
    if (allocResult != vk::Result::eSuccess)
    {
        EZASSERT(false, "Failed to allocate descriptor set!");
        return false;
    }

    vk::DescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = hardcodedGlobalUBOSize;

    vk::WriteDescriptorSet descriptorWrite = {};
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    logicalDevice.updateDescriptorSets({ descriptorWrite }, {});

    return true;
}
}  // namespace ez
