//
//  object.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 21/01/2021.
//

#include "object.hpp"

namespace pvk {
    Object::Object() = default;

    Object::~Object() = default;

    auto Object::createFromGLTF(vk::Queue &graphicsQueue, const std::string &filename) -> std::unique_ptr<Object> {
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
                                                                     vk::UniqueDeviceMemory &memory)> &function) const {
        for (auto &node : this->gltfObject->nodeLookup) {
            auto &uniformBuffersMemory = node.second->uniformBuffersMemory[bindingIndex];

            for (auto &uniformBufferMemory : uniformBuffersMemory) {
                function(*this->gltfObject, *node.second, uniformBufferMemory);
            }
        }
    }

    const std::vector<std::shared_ptr<gltf::Node>> &Object::getNodes() const {
        return this->gltfObject->nodes;
    }

    auto Object::getAnimation(uint32_t animationIndex) const -> const gltf::Animation & {
        return *this->gltfObject->animations[animationIndex];
    }

    auto Object::getAnimation(uint32_t animationIndex) -> gltf::Animation & {
        return *this->gltfObject->animations[animationIndex];
    }
}
