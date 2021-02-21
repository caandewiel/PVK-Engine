//
//  PvkGLTFNode.hpp
//  PVK
//
//  Created by Christian aan de Wiel on 10/01/2021.
//

#ifndef PvkGLTFNode_hpp
#define PvkGLTFNode_hpp

#include <cstdio>
#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "../buffer/buffer.hpp"
#include "../mesh/mesh.hpp"
#include "../util/util.hpp"
#include "GLTFPrimitive.hpp"

namespace pvk::gltf
{
class Node : pvk::util::NoCopy
{
public:
    Node() = default;
    ~Node() = default;

    Node(Node &&other) = default;
    Node &operator=(Node &&other) = default;

    [[nodiscard]] glm::mat4 getGlobalMatrix() const;
    [[nodiscard]] glm::mat4 getLocalMatrix() const;
    [[nodiscard]] std::vector<vk::DescriptorSet> getDescriptorSetsBySwapChainIndex(uint32_t swapChainIndex) const;

    // DescriptorBufferInfo
    [[nodiscard]] const std::map<uint32_t, std::map<uint32_t, std::vector<vk::DescriptorBufferInfo>>>
        &getDescriptorBuffersInfo() const;
    [[nodiscard]] const vk::DescriptorBufferInfo &getDescriptorBufferInfo(uint32_t descriptorSetIndex,
                                                                          uint32_t bindingIndex,
                                                                          uint32_t swapChainIndex);

    // Buffer
    [[nodiscard]] const vk::UniqueBuffer &getUniformBuffer(uint32_t descriptorSetIndex,
                                                           uint32_t bindingIndex,
                                                           uint32_t swapChainIndex) const;
    [[nodiscard]] vk::UniqueBuffer &getUniformBuffer(uint32_t descriptorSetIndex,
                                                     uint32_t bindingIndex,
                                                     uint32_t swapChainIndex);
    [[nodiscard]] std::vector<vk::UniqueBuffer> &getUniformBuffers(uint32_t descriptorSetIndex, uint32_t bindingIndex);

    // DeviceMemory
    [[nodiscard]] const vk::UniqueDeviceMemory &getUniformBufferMemory(uint32_t descriptorSetIndex,
                                                                       uint32_t bindingIndex,
                                                                       uint32_t swapChainIndex) const;
    [[nodiscard]] vk::UniqueDeviceMemory &getUniformBufferMemory(uint32_t descriptorSetIndex,
                                                                 uint32_t bindingIndex,
                                                                 uint32_t swapChainIndex);
    [[nodiscard]] std::vector<vk::UniqueDeviceMemory> &getUniformBuffersMemory(uint32_t descriptorSetIndex,
                                                                               uint32_t bindingIndex);


    void setDescriptorBufferInfo(vk::DescriptorBufferInfo &&descriptorBufferInfo,
                                 uint32_t descriptorSetIndex,
                                 uint32_t bindingIndex,
                                 uint32_t swapChainIndex);

    std::vector<std::shared_ptr<Node>> children;
    std::vector<std::unique_ptr<Primitive>> primitives;
    std::unique_ptr<Mesh> mesh;
    std::weak_ptr<Node> parent;
    int skinIndex = -1;
    int nodeIndex = -1;
    std::string name;

    struct
    {
        glm::mat4 model;
        glm::mat4 localMatrix;
        glm::mat4 inverseBindMatrices[256];
        float jointCount;
    } bufferObject{};

    std::vector<std::vector<vk::UniqueDescriptorSet>> descriptorSets{};

    glm::vec3 translation = glm::vec3(0.0F);
    glm::vec3 scale = glm::vec3(1.0F);
    glm::mat4 rotation = glm::mat4(1.0F);
    glm::mat4 matrix = glm::mat4(1.0F);

private:
    std::map<uint32_t, std::map<uint32_t, std::vector<vk::UniqueBuffer>>> uniformBuffers;
    std::map<uint32_t, std::map<uint32_t, std::vector<vk::UniqueDeviceMemory>>> uniformBuffersMemory;
    std::map<uint32_t, std::map<uint32_t, std::vector<vk::DescriptorBufferInfo>>> descriptorBuffersInfo;
};
} // namespace pvk::gltf

#endif /* PvkGLTFNode_hpp */
