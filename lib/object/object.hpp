//
//  object.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 21/01/2021.
//

#ifndef object_hpp
#define object_hpp

#include <cstdio>
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
    static auto createFromGLTF(vk::Queue &graphicsQueue, const std::string &filename) -> std::unique_ptr<Object>;

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
        for (auto &node : this->gltfObject->nodeLookup)
        {
            auto &uniformBuffersMemory = node.second->getUniformBuffersMemory(descriptorSetIndex, bindingIndex);

            for (auto &uniformBufferMemory : uniformBuffersMemory)
            {
                function(*this->gltfObject, *node.second, uniformBufferMemory);
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
