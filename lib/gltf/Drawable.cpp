//
// Created by Christian aan de Wiel on 22/02/2021.
//

#include "Drawable.h"

namespace pvk
{

std::vector<vk::DescriptorSet> Drawable::getDescriptorSetsBySwapChainIndex(uint32_t swapChainIndex) const
{
    std::vector<vk::DescriptorSet> result{};

    for (const auto &descriptorSet : this->descriptorSets)
    {
        result.emplace_back(descriptorSet.second[swapChainIndex].get());
    }

    return result;
}

const std::map<uint32_t, std::map<uint32_t, std::vector<vk::DescriptorBufferInfo>>>
    &Drawable::getDescriptorBuffersInfo() const
{
    return descriptorBuffersInfo;
}

void Drawable::setDescriptorBufferInfo(vk::DescriptorBufferInfo &&descriptorBufferInfo,
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

const vk::DescriptorBufferInfo &Drawable::getDescriptorBufferInfo(uint32_t descriptorSetIndex,
                                                                  uint32_t bindingIndex,
                                                                  uint32_t swapChainIndex)
{
    return this->descriptorBuffersInfo[descriptorSetIndex][bindingIndex][swapChainIndex];
}

const vk::UniqueBuffer &Drawable::getUniformBuffer(uint32_t descriptorSetIndex,
                                                   uint32_t bindingIndex,
                                                   uint32_t swapChainIndex) const
{
    return this->uniformBuffers.at(descriptorSetIndex).at(bindingIndex).at(swapChainIndex);
}

vk::UniqueBuffer &Drawable::getUniformBuffer(uint32_t descriptorSetIndex,
                                             uint32_t bindingIndex,
                                             uint32_t swapChainIndex)
{
    try
    {
        return this->uniformBuffers.at(descriptorSetIndex).at(bindingIndex).at(swapChainIndex);
    }
    catch (std::exception &exception)
    {
        throw std::runtime_error((std::ostringstream()
                                  << "[[DRAWABLE_NODE]] "
                                  << "No UniformBuffer found at descriptor set " << descriptorSetIndex << ", binding "
                                  << bindingIndex << ", swapchain index " << swapChainIndex)
                                     .str());
    }
}

std::vector<vk::UniqueBuffer> &Drawable::getUniformBuffers(uint32_t descriptorSetIndex, uint32_t bindingIndex)
{
    return this->uniformBuffers[descriptorSetIndex][bindingIndex];
}

std::vector<vk::UniqueDeviceMemory> &Drawable::getUniformBuffersMemory(uint32_t descriptorSetIndex,
                                                                       uint32_t bindingIndex)
{
    return this->uniformBuffersMemory[descriptorSetIndex][bindingIndex];
}

const vk::UniqueDeviceMemory &Drawable::getUniformBufferMemory(uint32_t descriptorSetIndex,
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
                                  << "[[DRAWABLE_NODE]] "
                                  << "No DeviceMemory found at descriptor set " << descriptorSetIndex << ", binding "
                                  << bindingIndex << ", swapchain index " << swapChainIndex)
                                     .str());
    }
}

vk::UniqueDeviceMemory &Drawable::getUniformBufferMemory(uint32_t descriptorSetIndex,
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

void Drawable::addUniformBufferToDescriptorSet(const vk::DescriptorSetLayoutBinding &descriptor,
                                               size_t uniformBufferSize,
                                               uint32_t descriptorSetIndex,
                                               uint32_t swapChainImageIndex)
{
    this->getUniformBuffers(descriptorSetIndex, descriptor.binding).resize(Context::getNumberOfSwapChainImages());
    this->getUniformBuffersMemory(descriptorSetIndex, descriptor.binding).resize(Context::getNumberOfSwapChainImages());

    pvk::buffer::create(uniformBufferSize,
                        vk::BufferUsageFlagBits::eUniformBuffer,
                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                        this->getUniformBuffer(descriptorSetIndex, descriptor.binding, swapChainImageIndex),
                        this->getUniformBufferMemory(descriptorSetIndex, descriptor.binding, swapChainImageIndex));
}

const std::map<uint32_t, std::vector<vk::UniqueDescriptorSet>> &Drawable::getDescriptorSets() const
{
    return this->descriptorSets;
}

std::map<uint32_t, std::vector<vk::UniqueDescriptorSet>> &Drawable::getDescriptorSets()
{
    return this->descriptorSets;
}

const std::vector<vk::UniqueDescriptorSet> &Drawable::getDescriptorSets(uint32_t descriptorSetIndex) const
{
    try
    {
        return this->descriptorSets.at(descriptorSetIndex);
    }
    catch (std::exception &exception)
    {
        throw std::runtime_error((std::ostringstream() << "[[DRAWABLE_NODE]] "
                                                       << "No DescriptorSet found at " << descriptorSetIndex)
                                     .str());
    }
}

std::vector<vk::UniqueDescriptorSet> &Drawable::getDescriptorSets(uint32_t descriptorSetIndex)
{
    try
    {
        return this->descriptorSets.at(descriptorSetIndex);
    }
    catch (std::exception &exception)
    {
        throw std::runtime_error((std::ostringstream() << "[[DRAWABLE_NODE]] "
                                                       << "No DescriptorSet found at " << descriptorSetIndex)
                                     .str());
    }
}

const vk::UniqueDescriptorSet &Drawable::getDescriptorSet(uint32_t descriptorSetIndex,
                                                          uint32_t swapChainImageIndex) const
{
    try
    {
        return this->descriptorSets.at(descriptorSetIndex).at(swapChainImageIndex);
    }
    catch (std::exception &exception)
    {
        throw std::runtime_error((std::ostringstream() << "[[DRAWABLE_NODE]] "
                                                       << "No DescriptorSet found at " << descriptorSetIndex
                                                       << " for swapchain index " << swapChainImageIndex)
                                     .str());
    }
}

void Drawable::initializeDescriptorSets(vk::DescriptorSetAllocateInfo &descriptorSetAllocateInfo,
                                        uint32_t descriptorSetIndex)
{
    this->descriptorSets.at(descriptorSetIndex).resize(Context::getNumberOfSwapChainImages());
    this->descriptorSets.at(descriptorSetIndex) =
        Context::getLogicalDevice().allocateDescriptorSetsUnique(descriptorSetAllocateInfo);
}

} // namespace pvk
