#include "mesh.hpp"

#include "render/vulkan/vulkan_buffer.hpp"

namespace ez {

    Mesh GetTestMesh(int sceneIndex)
    {
        Mesh mesh;

        const float offset = sceneIndex > 0 ? 0.5f : 0.0f;
        mesh.vertices = {
            {{-0.5f + offset, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f + offset, -0.5f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f + offset, 0.5f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f + offset, 0.5f}, {1.0f, 1.0f, 1.0f}}
        };

        mesh.indices = {
            0, 1, 2, 2, 3, 0
        };

        return mesh;
    }

    Mesh::~Mesh()
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

    bool Mesh::CreateVertexBuffers(vk::Device aLogicalDevice, vk::PhysicalDevice physicalDevice,
                                   vk::Queue graphicsQueue, vk::CommandPool graphicsCommandPool)
    {
        // todo: set logicalDevice from outside explicitly, stop passing it (and other stuff)
        logicalDevice = aLogicalDevice;
        vk::DeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
        vk::DeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();

        VulkanBuffer::createBuffer(logicalDevice, physicalDevice, vertexBufferSize,
                                   vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                                   vk::MemoryPropertyFlagBits::eDeviceLocal,
                                   vertexBuffer, vertexBufferMemory);
        VulkanBuffer::createBuffer(logicalDevice, physicalDevice, indexBufferSize,
                                   vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                                   vk::MemoryPropertyFlagBits::eDeviceLocal,
                                   indexBuffer, indexBufferMemory);
        VulkanBuffer::createBuffer(logicalDevice, physicalDevice, uniformBufferMaxHackSize,
                                   vk::BufferUsageFlagBits::eUniformBuffer,
                                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                   uniformBuffer, uniformBufferMemory);

        // vert
        {
            vk::Buffer stagingBuffer;
            vk::DeviceMemory stagingBufferMemory;
            VulkanBuffer::createBuffer(logicalDevice, physicalDevice,
                        vertexBufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                         stagingBuffer, stagingBufferMemory);

            void* data;

            logicalDevice.mapMemory(stagingBufferMemory, 0, vertexBufferSize, vk::MemoryMapFlags(), &data);
            memcpy(data, vertices.data(), static_cast<size_t>(vertexBufferSize));
            logicalDevice.unmapMemory(stagingBufferMemory);

            VulkanBuffer::copyBuffer(logicalDevice, graphicsQueue, graphicsCommandPool, stagingBuffer, vertexBuffer, vertexBufferSize);

            logicalDevice.destroyBuffer(stagingBuffer);
            logicalDevice.freeMemory(stagingBufferMemory);
        }

        // index
        {
            vk::Buffer stagingBuffer;
            vk::DeviceMemory stagingBufferMemory;
            VulkanBuffer::createBuffer(logicalDevice, physicalDevice,
                         indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                          stagingBuffer, stagingBufferMemory);

            void* data;
            logicalDevice.mapMemory(stagingBufferMemory, 0, indexBufferSize, vk::MemoryMapFlags(), &data);
            memcpy(data, indices.data(), static_cast<size_t>(indexBufferSize));
            logicalDevice.unmapMemory(stagingBufferMemory);

            VulkanBuffer::copyBuffer(logicalDevice, graphicsQueue, graphicsCommandPool, stagingBuffer, indexBuffer, indexBufferSize);

            logicalDevice.destroyBuffer(stagingBuffer);
            logicalDevice.freeMemory(stagingBufferMemory);
        }

        return true;
    }

    bool Mesh::CreateDescriptorSet(vk::Device logicalDevice, vk::DescriptorPool descriptorPool, vk::DescriptorSetLayout descriptorSetLayout, size_t hardcodedGlobalUBOSize)
    {
        // todo: set logicalDevice from outside explicitly, stop passing it (and other stuff)
        vk::DescriptorSetLayout layouts[] = {descriptorSetLayout};
        vk::DescriptorSetAllocateInfo allocInfo = {};
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = layouts;

        vk::Result allocResult = logicalDevice.allocateDescriptorSets(&allocInfo, &descriptorSet);
        if (allocResult != vk::Result::eSuccess) {
            printf("Failed to allocate descriptor set!");
            assert(false);
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

        logicalDevice.updateDescriptorSets({descriptorWrite}, {});

        return true;
    }
}
