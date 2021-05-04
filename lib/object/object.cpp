//
//  object.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 21/01/2021.
//

#include "object.hpp"

namespace pvk
{
Object::Object() = default;

Object::~Object() = default;

std::unique_ptr<Object> Object::createFromGLTF(vk::Queue &&graphicsQueue, const std::string &filename)
{
    auto object = std::unique_ptr<Object>(new Object());

    object->gltfObject = pvk::GLTFLoader::loadObject(graphicsQueue, filename);

    return object;
}

void Object::updateUniformBuffer(void *data, size_t size, uint32_t descriptorSetIndex, uint32_t bindingIndex) const
{
    for (const auto &node : this->gltfObject->getNodes())
    {
        auto &uniformBuffersMemory = node.second->getUniformBuffersMemory(descriptorSetIndex, bindingIndex);

        for (auto &uniformBufferMemory : uniformBuffersMemory)
        {
            pvk::buffer::update(uniformBufferMemory, size, data);
        }
    }
}

const gltf::Animation &Object::getAnimation(uint32_t animationIndex) const
{
    return *this->gltfObject->animations[animationIndex];
}

gltf::Animation &Object::getAnimation(uint32_t animationIndex)
{
    return *this->gltfObject->animations[animationIndex];
}
} // namespace pvk
