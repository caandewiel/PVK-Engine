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

    std::unique_ptr<Object> Object::createFromGLTF(vk::Queue &graphicsQueue, const std::string &filename) {
        auto object = std::unique_ptr<Object>(new Object());

        object->gltfObject = pvk::GLTFLoader::loadObject(graphicsQueue, filename);

        return object;
    }

    void Object::updateUniformBuffer(uint32_t bindingIndex, size_t size, void *data) const {
        for (auto &node : this->gltfObject->nodeLookup) {
            auto &uniformBuffersMemory = node.second->uniformBuffersMemory[bindingIndex];

            for (auto &uniformBufferMemory : uniformBuffersMemory) {
                pvk::buffer::update(uniformBufferMemory, size, data);
            }
        }
    }

    void Object::updateUniformBufferPerNode(uint32_t bindingIndex,
                                            const std::function<void(pvk::gltf::Object &object,
                                                                     pvk::gltf::Node &node,
                                                                     vk::DeviceMemory &memory)> &function) const {
        for (auto &node : this->gltfObject->nodeLookup) {
            auto &uniformBuffersMemory = node.second->uniformBuffersMemory[bindingIndex];

            for (auto &uniformBufferMemory : uniformBuffersMemory) {
                function(*this->gltfObject, *node.second, uniformBufferMemory);
            }
        }
    }

    const std::vector<std::shared_ptr<gltf::Node>> & Object::getNodes() const {
        return this->gltfObject->nodes;
    }

    auto Object::getAnimation(uint32_t animationIndex) const -> const gltf::Animation & {
        return *this->gltfObject->animations[animationIndex];
    }

    auto Object::getAnimation(uint32_t animationIndex) -> gltf::Animation & {
        return *this->gltfObject->animations[animationIndex];
    }
}
