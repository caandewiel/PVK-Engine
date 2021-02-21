//
//  PvkGLTFNode.cpp
//  PVK
//
//  Created by Christian aan de Wiel on 10/01/2021.
//

#include "GLTFNode.hpp"

namespace pvk::gltf
{
glm::mat4 Node::getGlobalMatrix() const
{
    auto localMatrix = this->getLocalMatrix();
    auto currentParent = this->parent;

    while (currentParent.lock())
    {
        localMatrix = currentParent.lock()->getLocalMatrix() * localMatrix;
        currentParent = currentParent.lock()->parent;
    }

    return localMatrix;
}

glm::mat4 Node::getLocalMatrix() const
{
    if (this->matrix == glm::mat4(1.0F))
    {
        return glm::translate(glm::mat4(1.0F), this->translation) * glm::mat4(this->rotation) *
               glm::scale(glm::mat4(1.0F), this->scale) * this->matrix;
    }

    return this->matrix;
}

std::vector<vk::DescriptorSet> Node::getDescriptorSetsBySwapChainIndex(uint32_t swapChainIndex) const
{
    std::vector<vk::DescriptorSet> result{};

    for (const auto &descriptorSet : this->descriptorSets)
    {
        result.emplace_back(descriptorSet[swapChainIndex].get());
    }

    return result;
}

const std::map<uint32_t, std::map<uint32_t, std::vector<vk::DescriptorBufferInfo>>> &Node::getDescriptorBuffersInfo()
    const
{
    return descriptorBuffersInfo;
}

void Node::setDescriptorBufferInfo(vk::DescriptorBufferInfo &&descriptorBufferInfo,
                                   uint32_t descriptorSetIndex,
                                   uint32_t bindingIndex,
                                   uint32_t swapChainIndex)
{
    if (this->descriptorBuffersInfo[descriptorSetIndex][bindingIndex].size() < Context::getNumberOfSwapChainImages())
    {
        this->descriptorBuffersInfo[descriptorSetIndex][bindingIndex].resize(Context::getNumberOfSwapChainImages());
    }

    this->descriptorBuffersInfo[descriptorSetIndex][bindingIndex][swapChainIndex] = descriptorBufferInfo;
}

const vk::DescriptorBufferInfo &Node::getDescriptorBufferInfo(uint32_t descriptorSetIndex,
                                                              uint32_t bindingIndex,
                                                              uint32_t swapChainIndex)
{
    return this->descriptorBuffersInfo[descriptorSetIndex][bindingIndex][swapChainIndex];
}

const vk::UniqueBuffer &Node::getUniformBuffer(uint32_t descriptorSetIndex,
                                               uint32_t bindingIndex,
                                               uint32_t swapChainIndex) const
{
    return this->uniformBuffers.at(descriptorSetIndex).at(bindingIndex).at(swapChainIndex);
}

vk::UniqueBuffer &Node::getUniformBuffer(uint32_t descriptorSetIndex, uint32_t bindingIndex, uint32_t swapChainIndex)
{
    try
    {
        return this->uniformBuffers.at(descriptorSetIndex).at(bindingIndex).at(swapChainIndex);
    }
    catch (std::exception &exception)
    {
        throw std::runtime_error((std::ostringstream()
                                  << "[[NODE]] "
                                  << "No UniformBuffer found at descriptor set " << descriptorSetIndex
                                  << ", binding " << bindingIndex
                                  << ", swapchain index " << swapChainIndex).str());
    }
}

std::vector<vk::UniqueBuffer> &Node::getUniformBuffers(uint32_t descriptorSetIndex, uint32_t bindingIndex)
{
    return this->uniformBuffers[descriptorSetIndex][bindingIndex];
}

std::vector<vk::UniqueDeviceMemory> &Node::getUniformBuffersMemory(uint32_t descriptorSetIndex, uint32_t bindingIndex)
{
    return this->uniformBuffersMemory[descriptorSetIndex][bindingIndex];
}

const vk::UniqueDeviceMemory &Node::getUniformBufferMemory(uint32_t descriptorSetIndex,
                                                           uint32_t bindingIndex,
                                                           uint32_t swapChainIndex) const
{
    try
    {
        return this->uniformBuffersMemory.at(descriptorSetIndex).at(bindingIndex).at(swapChainIndex);
    }
    catch (std::exception &exception)
    {
        throw std::runtime_error((std::ostringstream()
                                  << "[[NODE]] "
                                  << "No DeviceMemory found at descriptor set " << descriptorSetIndex << ", binding "
                                  << bindingIndex << ", swapchain index " << swapChainIndex).str());
    }
}

vk::UniqueDeviceMemory &Node::getUniformBufferMemory(uint32_t descriptorSetIndex,
                                                     uint32_t bindingIndex,
                                                     uint32_t swapChainIndex)
{
    try
    {
        return this->uniformBuffersMemory.at(descriptorSetIndex).at(bindingIndex).at(swapChainIndex);
    }
    catch (std::exception &exception)
    {
        throw std::runtime_error((std::ostringstream()
                                  << "No Buffer found at descriptor set " << descriptorSetIndex << ", binding "
                                  << bindingIndex << ", swapchain index" << swapChainIndex)
                                     .str());
    }
}
} // namespace pvk::gltf
