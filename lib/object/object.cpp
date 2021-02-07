//
//  object.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 21/01/2021.
//

#include "object.hpp"

namespace pvk {
    Object::Object() {}

    Object::~Object() {
        Context::getLogicalDevice().destroyBuffer(this->gltfObject->vertexBuffer, nullptr);
        Context::getLogicalDevice().destroyBuffer(this->gltfObject->indexBuffer, nullptr);
        Context::getLogicalDevice().freeMemory(this->gltfObject->vertexBufferMemory, nullptr);
        Context::getLogicalDevice().freeMemory(this->gltfObject->indexBufferMemory, nullptr);

        for (auto &node : this->gltfObject->nodes) {
            auto itUniformBuffer = node->uniformBuffers.begin();
            while (itUniformBuffer != node->uniformBuffers.end()) {
                for (auto &uniformBuffer : itUniformBuffer->second) {
                    Context::getLogicalDevice().destroyBuffer(uniformBuffer, nullptr);
                }

                itUniformBuffer++;
            }

            auto itUniformBufferMemory = node->uniformBuffersMemory.begin();
            while (itUniformBufferMemory != node->uniformBuffersMemory.end()) {
                for (auto &uniformBufferMemory : itUniformBufferMemory->second) {
                    Context::getLogicalDevice().freeMemory(uniformBufferMemory, nullptr);
                }

                itUniformBufferMemory++;
            }
        }
    }

    Object *Object::createFromGLTF(vk::Queue &graphicsQueue, const std::string &filename) {
        auto *object = new Object();

        object->gltfObject = pvk::GLTFLoader::loadObject(graphicsQueue, filename);

        return object;
    }

    void Object::updateUniformBuffer(uint32_t bindingIndex, size_t size, void *data) const {
        for (auto &node : this->gltfObject->nodes) {
            auto &uniformBuffersMemory = node->uniformBuffersMemory[bindingIndex];

            for (auto &uniformBufferMemory : uniformBuffersMemory) {
                pvk::buffer::update(uniformBufferMemory, size, data);
            }
        }
    }

    void Object::updateUniformBufferPerNode(uint32_t bindingIndex,
                                            const std::function<void(pvk::gltf::Object *object,
                                                                     pvk::gltf::Node *node,
                                                                     vk::DeviceMemory &memory)> &function) const {
        for (auto &node : this->gltfObject->nodes) {
            auto &uniformBuffersMemory = node->uniformBuffersMemory[bindingIndex];

            for (auto &uniformBufferMemory : uniformBuffersMemory) {
                function(this->gltfObject, node, uniformBufferMemory);
            }
        }
    }

    void Object::updateUniformBufferPerNode(uint32_t bindingIndex,
                                            std::function<void(pvk::gltf::Node *node,
                                                               vk::DeviceMemory &memory,
                                                               void *data)> function,
                                            void *data) {
        for (auto &node : this->gltfObject->nodes) {
            auto &uniformBuffersMemory = node->uniformBuffersMemory[bindingIndex];

            for (auto &uniformBufferMemory : uniformBuffersMemory) {
                function(node, uniformBufferMemory, data);
            }
        }
    }

    std::vector<gltf::Node *> Object::getNodes() const {
        return this->gltfObject->nodes;
    }

    std::vector<gltf::Animation *> Object::getAnimations() const {
        return this->gltfObject->animations;
    }
}
