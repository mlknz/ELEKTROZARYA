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
Model::Model(eType type, const std::string& filePath)
{
    name = filePath;

    if (type == eType::GltfMesh)
    {
        const std::string gltfFilePath = filePath;

        EZLOG("loading gltf file", gltfFilePath);
        tinygltf::Model gltfModel;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;

        bool fileLoaded = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, gltfFilePath);

        EZASSERT(fileLoaded, "Failed to load file");

        for (const tinygltf::Sampler& gltfSampler : gltfModel.samplers)
        {
            textureSamplers.push_back(TextureSampler::FromGltfSampler(gltfSampler.magFilter,
                                                                      gltfSampler.minFilter,
                                                                      gltfSampler.wrapS,
                                                                      gltfSampler.wrapT));
        }
        textures.reserve(12);
        for (tinygltf::Texture& tex : gltfModel.textures)
        {
            const size_t imageIndex = static_cast<size_t>(tex.source);
            const size_t samplerIndex = static_cast<size_t>(tex.sampler);

            tinygltf::Image gltfImage = gltfModel.images.at(imageIndex);
            TextureSampler textureSampler =
                (tex.sampler >= 0) ? textureSamplers.at(samplerIndex) : TextureSampler{};

            TextureCreationInfo textureCI =
                TextureCreationInfo::CreateFromData(&gltfImage.image.at(0),
                                                    static_cast<uint32_t>(gltfImage.width),
                                                    static_cast<uint32_t>(gltfImage.height),
                                                    static_cast<uint32_t>(gltfImage.component),
                                                    1,
                                                    true,
                                                    textureSampler);
            textures.emplace_back(std::move(textureCI));  // textures are loaded to GPU later
        }
        LoadMaterials(gltfModel);

        indices = {};
        vertices = {};
        vertexShaderName = "../source/shaders/shader.vert";
        fragmentShaderName = "../source/shaders/shader.frag";
        vertexLayout = eVertexLayout::vlPosition | eVertexLayout::vlNormal |
                       eVertexLayout::vlTexcoord0 | eVertexLayout::vlTexcoord1;

        const tinygltf::Scene& scene = gltfModel.scenes.at(size_t(gltfModel.defaultScene));
        for (size_t i = 0; i < scene.nodes.size(); i++)
        {
            const tinygltf::Node node = gltfModel.nodes.at(size_t(scene.nodes[i]));
            EZLOG("Loading Node", node.name);
            LoadNodeFromGLTF(
                nullptr, node, uint32_t(scene.nodes[i]), gltfModel, indices, vertices);
        }
    }
    else if (type == eType::Cubemap)
    {
        // tex0 - panorama HDR
        // tex1 - cubemap
        const std::string hdrPanoramaFilePath = filePath;
        EZLOG("loading hdr panorama file", hdrPanoramaFilePath);
        StbImageLoader::StbImageWrapper hdrPanoramaWrapper =
            StbImageLoader::LoadHDRImage(hdrPanoramaFilePath.c_str());
        TextureSampler panoramaTexSampler = {};
        TextureCreationInfo panoramaTexCI = TextureCreationInfo::CreateHdrFromData(
            hdrPanoramaWrapper.hdrData,
            static_cast<uint32_t>(hdrPanoramaWrapper.width),
            static_cast<uint32_t>(hdrPanoramaWrapper.height),
            static_cast<uint32_t>(hdrPanoramaWrapper.channelsCount),
            panoramaTexSampler);
        textures.emplace_back(std::move(panoramaTexCI));

        uint32_t cubemapWidth = 512;
        uint32_t cubemapHeight = 512;
        const uint32_t cubemapColorChannelsCount = 4;
        uint32_t singleFaceSize = cubemapWidth * cubemapWidth * cubemapColorChannelsCount;

        const uint32_t cubemapImageLayersCount = 6;

        std::array<std::array<uint8_t, cubemapColorChannelsCount>, cubemapImageLayersCount>
            faceColors;
        faceColors[0] = { 255, 0, 0, 255 };
        faceColors[1] = { 128, 0, 0, 255 };
        faceColors[2] = { 0, 255, 0, 255 };
        faceColors[3] = { 0, 128, 0, 255 };
        faceColors[4] = { 0, 0, 255, 255 };
        faceColors[5] = { 0, 0, 128, 255 };

        std::vector<uint8_t> cubemapData;
        cubemapData.resize(singleFaceSize * cubemapImageLayersCount);
        for (uint32_t face = 0; face < cubemapImageLayersCount; ++face)
        {
            for (uint32_t i = 0; i < singleFaceSize; i += cubemapColorChannelsCount)
            {
                cubemapData[face * singleFaceSize + i + 0] = faceColors[face][0];
                cubemapData[face * singleFaceSize + i + 1] = faceColors[face][1];
                cubemapData[face * singleFaceSize + i + 2] = faceColors[face][2];
                cubemapData[face * singleFaceSize + i + 3] = faceColors[face][3];
            }
        }

        TextureSampler cubemapTexSampler = {};
        TextureCreationInfo cubemapTexCI =
            TextureCreationInfo::CreateFromData(cubemapData.data(),
                                                cubemapWidth,
                                                cubemapHeight,
                                                cubemapColorChannelsCount,
                                                cubemapImageLayersCount,
                                                false,
                                                cubemapTexSampler);
        cubemapTexCI.SetIsCubemap(true);
        textures.emplace_back(std::move(cubemapTexCI));

        vertexShaderName = "../source/shaders/env_cubemap.vert";
        fragmentShaderName = "../source/shaders/env_cubemap.frag";
        vertexLayout = eVertexLayout::vlPosition;

        glm::vec3 fakeN = glm::vec3(0, 0, 1);
        glm::vec2 zeroVec2 = glm::zero<glm::vec2>();
        vertices = {
            { glm::vec3(-1, -1, -1), fakeN, zeroVec2, zeroVec2 },
            { glm::vec3(1, -1, -1), fakeN, zeroVec2, zeroVec2 },
            { glm::vec3(1, 1, -1), fakeN, zeroVec2, zeroVec2 },
            { glm::vec3(-1, 1, -1), fakeN, zeroVec2, zeroVec2 },
            { glm::vec3(-1, -1, 1), fakeN, zeroVec2, zeroVec2 },
            { glm::vec3(1, -1, 1), fakeN, zeroVec2, zeroVec2 },
            { glm::vec3(1, 1, 1), fakeN, zeroVec2, zeroVec2 },
            { glm::vec3(-1, 1, 1), fakeN, zeroVec2, zeroVec2 },
        };
        indices = { 0, 1, 3, 3, 1, 2,  //
                    1, 5, 2, 2, 5, 6,  //
                    5, 4, 6, 6, 4, 7,  //
                    4, 0, 7, 7, 0, 3,  //
                    3, 2, 7, 7, 2, 6,  //
                    4, 5, 0, 0, 5, 1 };

        materials.emplace_back();
        Material& cubemapMat = materials.back();
        cubemapMat.type = MaterialType::eCubemap;
        cubemapMat.cubemapTexture = &textures[1];

        std::unique_ptr<Node> envCubemapNode = std::make_unique<Node>();
        envCubemapNode->name = "env cubemap";
        envCubemapNode->aabb = BoundingBox(glm::vec3(-10000), glm::vec3(10000));
        envCubemapNode->mesh = std::make_unique<Mesh>(glm::mat4(1.0f));
        envCubemapNode->mesh->bb = envCubemapNode->aabb;
        std::unique_ptr<Primitive> primitive =
            std::make_unique<Primitive>(0, indices.size(), vertices.size(), materials.back());
        envCubemapNode->mesh->primitives.push_back(std::move(primitive));
        nodes.push_back(std::move(envCubemapNode));
    }
    else
    {
        EZASSERT(false, "Unsupported Model Type");
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

    // Generate local node matrix
    if (node.translation.size() == 3)
    {
        glm::vec3 translation = glm::make_vec3(node.translation.data());
        newNode->translation = translation;
    }
    if (node.rotation.size() == 4)
    {
        glm::quat q = glm::make_quat(node.rotation.data());
        newNode->rotation = glm::mat4(q);
    }
    if (node.scale.size() == 3)
    {
        glm::vec3 scale = glm::make_vec3(node.scale.data());
        newNode->scale = scale;
    }
    if (node.matrix.size() == 16) { newNode->matrix = glm::make_mat4x4(node.matrix.data()); }

    // Node with children
    if (node.children.size() > 0)
    {
        for (size_t i = 0; i < node.children.size(); i++)
        {
            uint32_t childIndex = uint32_t(node.children[i]);
            LoadNodeFromGLTF(newNode.get(),
                             model.nodes.at(childIndex),
                             childIndex,
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
            Material& mat =
                primitive.material > -1 ? materials[primitive.material] : materials.back();
            std::unique_ptr<Primitive> newPrimitive =
                std::make_unique<Primitive>(indexStart, indexCount, vertexCount, mat);
            newPrimitive->SetBoundingBox(posMin, posMax);
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

void Model::LoadMaterials(tinygltf::Model& gltfModel)
{
    for (tinygltf::Material& mat : gltfModel.materials)
    {
        Material material{};
        if (mat.values.find("baseColorTexture") != mat.values.end())
        {
            material.textures.baseColor =
                &textures[mat.values["baseColorTexture"].TextureIndex()];
            material.texCoordSets.baseColor = mat.values["baseColorTexture"].TextureTexCoord();
        }
        if (mat.values.find("metallicRoughnessTexture") != mat.values.end())
        {
            material.textures.metallicRoughness =
                &textures[mat.values["metallicRoughnessTexture"].TextureIndex()];
            material.texCoordSets.metallicRoughness =
                mat.values["metallicRoughnessTexture"].TextureTexCoord();
        }

        if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end())
        {
            material.textures.normal =
                &textures[mat.additionalValues["normalTexture"].TextureIndex()];
            material.texCoordSets.normal =
                mat.additionalValues["normalTexture"].TextureTexCoord();
        }
        if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end())
        {
            material.textures.emission =
                &textures[mat.additionalValues["emissiveTexture"].TextureIndex()];
            material.texCoordSets.emissive =
                mat.additionalValues["emissiveTexture"].TextureTexCoord();
        }
        if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end())
        {
            material.textures.occlusion =
                &textures[mat.additionalValues["occlusionTexture"].TextureIndex()];
            material.texCoordSets.occlusion =
                mat.additionalValues["occlusionTexture"].TextureTexCoord();
        }
        if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end())
        {
            tinygltf::Parameter param = mat.additionalValues["alphaMode"];
            if (param.string_value == "BLEND") { material.blendMode = BlendMode::eAlphaBlend; }
            if (param.string_value == "MASK")
            {
                material.alphaCutoff = 0.5f;
                material.blendMode = BlendMode::eAlphaMask;
            }
        }
        materials.push_back(material);
    }
    materials.push_back(Material{});  // default
}

Model::~Model()
{
    if (logicalDevice)
    {
        logicalDevice.destroyBuffer(indexBuffer);
        logicalDevice.freeMemory(indexBufferMemory);

        logicalDevice.destroyBuffer(vertexBuffer);
        logicalDevice.freeMemory(vertexBufferMemory);
    }

    textures = {};
    textureSamplers = {};
}

bool Model::CreateVertexBuffers(vk::PhysicalDevice physicalDevice,
                                vk::Queue graphicsQueue,
                                vk::CommandPool graphicsCommandPool)
{
    EZASSERT(logicalDevice);
    EZASSERT(!vertices.empty(), "Model can't have empty vertices");
    EZASSERT(!indices.empty(), "Model can't have empty indices");
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

namespace StbImageLoader
{
StbImageWrapper::~StbImageWrapper()
{
    if (isHdr) { stbi_image_free(hdrData); }
}
StbImageWrapper LoadHDRImage(char const* path)
{
    StbImageWrapper result;
    result.isHdr = true;

    result.hdrData = stbi_loadf(path, &result.width, &result.height, &result.channelsCount, 0);
    return result;
}
}  // namespace StbImageLoader
}  // namespace ez
