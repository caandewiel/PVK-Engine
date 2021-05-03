//
//  object.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 21/01/2021.
//

#ifndef object_hpp
#define object_hpp


#include <future>
#include <glm/glm.hpp>
#include <string>
#include <vulkan/vulkan.hpp>

#include "../gltf/GLTFLoader.hpp"

namespace pvk
{
class Object
{
public:
    static auto createFromGLTF(vk::Queue &&graphicsQueue, const std::string &filename) -> std::unique_ptr<Object>;

    ~Object();

    [[nodiscard]] auto getAnimation(uint32_t animationIndex) const -> const gltf::Animation &;

    [[nodiscard]] auto getAnimation(uint32_t animationIndex) -> gltf::Animation &;

    void updateUniformBuffer(void *data, size_t size, uint32_t descriptorSetIndex, uint32_t bindingIndex) const;

    template<typename Fn>
    void updateUniformBufferPerNode(
        Fn &&function,
        uint32_t descriptorSetIndex,
        uint32_t bindingIndex) const
    {
        for (const auto &node : this->gltfObject->getNodes())
        {
            auto &uniformBuffersMemory = node.second.lock()->getUniformBuffersMemory(descriptorSetIndex, bindingIndex);

            for (auto &uniformBufferMemory : uniformBuffersMemory)
            {
                function(*this->gltfObject, *node.second.lock(), uniformBufferMemory);
            }
        }
    }

    template<typename Fn>
    void updateUniformBufferPerPrimitive(
        Fn &&function,
        uint32_t descriptorSetIndex,
        uint32_t bindingIndex) const
    {
        for (const auto &node : this->gltfObject->getNodes())
        {
            for (auto &primitive : node.second.lock()->primitives) {
                auto &uniformBuffersMemory = primitive->getUniformBuffersMemory(descriptorSetIndex, bindingIndex);

                for (auto &uniformBufferMemory : uniformBuffersMemory)
                {
                    function(*this->gltfObject, *primitive, uniformBufferMemory);
                }
            }
        }
    }

//    void writeUniformBuffersToGPU() {
//        for (auto &node : this->gltfObject->nodeLookup) {
//            for (auto &uniformBuffersMemoryMap : node.second->uniformBuffersMemory) {
//                for (auto &uniformBuffer : uniformBuffersMemoryMap.second) {
//                    for (auto &memory : uniformBuffer.second) {
//                        pvk::buffer::update(memory, sizeof(node.second->bufferObject), &node.second->bufferObject);
//                    }
//                }
//            }
//        }
//    }

    // @TODO: Make this private
    std::unique_ptr<gltf::Object> gltfObject;

private:
    Object();
};
} // namespace pvk

#endif /* object_hpp */
