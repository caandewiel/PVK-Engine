//
//  pipeline.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 21/01/2021.
//

#include "pipeline.hpp"

namespace pvk
{
Pipeline::~Pipeline()
{
    for (auto &object : this->objects)
    {
        object.reset();
    }

    for (auto &textureByDescriptorSet : this->textures)
    {
        for (auto &texture : textureByDescriptorSet)
        {
            texture.second.reset();
        }
    }
}

void Pipeline::registerObject(const std::shared_ptr<Object> &object)
{
    this->objects.emplace_back(object);
}

void Pipeline::registerTexture(const std::shared_ptr<Texture> &texture, uint8_t descriptorSetIndex, uint8_t binding)
{
    if (this->textures.size() < (descriptorSetIndex + 1))
    {
        this->textures.resize(descriptorSetIndex + 1);
    }

    this->textures[descriptorSetIndex][binding] = texture;
}

void Pipeline::prepare()
{
    auto numberOfSwapChainImages = static_cast<uint32_t>(Context::getNumberOfSwapChainImages());
    uint32_t numberOfUniformBuffers = 0;
    uint32_t numberOfCombinedImageSamplers = 0;
    uint32_t numberOfNodes = 0;

    // First count the amount of resources necessary for the descriptor pool
    getNumberOfResources(numberOfUniformBuffers, numberOfCombinedImageSamplers);

    for (auto &object : this->objects)
    {
        numberOfNodes += static_cast<uint32_t>(object->gltfObject->nodeLookup.size());
    }

    // Create the actual descriptor pool for this pipeline
    std::vector<vk::DescriptorPoolSize> poolSizes{
        {vk::DescriptorType::eUniformBuffer, numberOfNodes * numberOfUniformBuffers * numberOfSwapChainImages},
        {vk::DescriptorType::eCombinedImageSampler,
         numberOfNodes * numberOfCombinedImageSamplers * numberOfSwapChainImages},
    };

    // @TODO: Allow multiple descriptor pools per descriptor set visibility
    this->descriptorPool = Context::getLogicalDevice().createDescriptorPoolUnique(
        {{vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet},
         static_cast<uint32_t>(this->nodeDescriptorSetLayouts.size()) * numberOfNodes * numberOfSwapChainImages,
         static_cast<uint32_t>(poolSizes.size()),
         poolSizes.data()});

    for (auto &object : this->objects)
    {
        for (auto &node : object->gltfObject->nodeLookup)
        {
            node.second->descriptorSets.resize(nodeDescriptorSetLayouts.size());
        }

        for (size_t i = 0; i < nodeDescriptorSetLayouts.size(); i++)
        {
            object->gltfObject->initializeWriteDescriptorSets(Context::getLogicalDevice(),
                                                              this->descriptorPool.get(),
                                                              nodeDescriptorSetLayouts[i].get(),
                                                              numberOfSwapChainImages,
                                                              i);
        }
    }

    std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {};

    for (auto &object : this->objects)
    {
        for (auto &node : object->gltfObject->nodeLookup)
        {
            writeDescriptorSets = initializeDescriptorSet(numberOfSwapChainImages, writeDescriptorSets, *node.second);
        }
    }

    Context::getLogicalDevice().updateDescriptorSets(writeDescriptorSets, nullptr);
}

void Pipeline::getNumberOfResources(uint32_t numberOfUniformBuffers, uint32_t numberOfCombinedImageSamplers)
{
    for (auto &descriptorSetLayoutBindings : descriptorSetLayoutBindingsLookup)
    {
        for (auto &descriptor : descriptorSetLayoutBindings)
        {
            if (descriptor.descriptorType == vk::DescriptorType::eUniformBuffer)
            {
                numberOfUniformBuffers++;
            }
            else if (descriptor.descriptorType == vk::DescriptorType::eCombinedImageSampler)
            {
                numberOfCombinedImageSamplers++;
            }
            else
            {
                throw std::runtime_error("Unsupported descriptor type");
            }
        }
    }
}

const std::vector<vk::WriteDescriptorSet> &Pipeline::initializeDescriptorSet(
    uint32_t numberOfSwapchainImages,
    std::vector<vk::WriteDescriptorSet> &writeDescriptorSets,
    gltf::Node &node)
{
    for (uint32_t i = 0; i < numberOfSwapchainImages; i++)
    {
        for (size_t j = 0; j < descriptorSetLayoutBindingsLookup.size(); j++)
        {
            const auto &descriptorSetLayoutBindings = descriptorSetLayoutBindingsLookup[j];

            for (const auto &descriptor : descriptorSetLayoutBindings)
            {
                switch (descriptor.descriptorType)
                {
                case vk::DescriptorType::eUniformBuffer: {
                    addWriteDescriptorSetUniformBuffer(writeDescriptorSets, node, descriptor, j, i);
                    break;
                }
                case vk::DescriptorType::eCombinedImageSampler: {
                    addWriteDescriptorSetCombinedImageSampler(writeDescriptorSets, node, descriptor, j, i);
                    break;
                }
                default:
                    throw std::runtime_error("Unsupported descriptor type");
                }
            }
        }
    }
    return writeDescriptorSets;
}

void Pipeline::addWriteDescriptorSetUniformBuffer(std::vector<vk::WriteDescriptorSet> &writeDescriptorSets,
                                                  gltf::Node &node,
                                                  const vk::DescriptorSetLayoutBinding &descriptor,
                                                  uint32_t descriptorSetIndex,
                                                  uint32_t swapChainImageIndex)
{
    // It is important to note that the getUniformBuffers method creates a new entry if it doesn't exist in the map yet.
    node.getUniformBuffers(descriptorSetIndex, descriptor.binding).resize(Context::getNumberOfSwapChainImages());
    node.getUniformBuffersMemory(descriptorSetIndex, descriptor.binding).resize(Context::getNumberOfSwapChainImages());

    pvk::buffer::create(this->descriptorSetLayoutBindingSizesLookup[descriptorSetIndex][descriptor.binding],
                        vk::BufferUsageFlagBits::eUniformBuffer,
                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                        node.getUniformBuffer(descriptorSetIndex, descriptor.binding, swapChainImageIndex),
                        node.getUniformBufferMemory(descriptorSetIndex, descriptor.binding, swapChainImageIndex));

    node.setDescriptorBufferInfo(
        {node.getUniformBuffer(descriptorSetIndex, descriptor.binding, swapChainImageIndex).get(),
         0,
         this->descriptorSetLayoutBindingSizesLookup[descriptorSetIndex][descriptor.binding]},
        descriptorSetIndex,
        descriptor.binding,
        swapChainImageIndex);

    writeDescriptorSets.emplace_back(
        node.descriptorSets[descriptorSetIndex][swapChainImageIndex].get(),
        descriptor.binding,
        0,
        1,
        vk::DescriptorType::eUniformBuffer,
        nullptr,
        &node.getDescriptorBufferInfo(descriptorSetIndex, descriptor.binding, swapChainImageIndex));
}

void Pipeline::addWriteDescriptorSetCombinedImageSampler(std::vector<vk::WriteDescriptorSet> &writeDescriptorSets,
                                                         const gltf::Node &node,
                                                         const vk::DescriptorSetLayoutBinding &descriptor,
                                                         uint32_t descriptorSetIndex,
                                                         uint32_t swapChainImageIndex)
{
    writeDescriptorSets.emplace_back(
        node.descriptorSets[descriptorSetIndex][swapChainImageIndex].get(),
        descriptor.binding,
        0,
        1,
        vk::DescriptorType::eCombinedImageSampler,
        this->textures[descriptorSetIndex][descriptor.binding].lock()->getDescriptorImageInfo());
}

void Pipeline::setNodeDescriptorSetLayouts(std::vector<vk::UniqueDescriptorSetLayout> &&newDescriptorSetLayouts)
{
    this->nodeDescriptorSetLayouts = std::move(newDescriptorSetLayouts);
}

void Pipeline::setDescriptorSetLayoutBindingsLookup(
    std::vector<std::vector<vk::DescriptorSetLayoutBinding>> &&newDescriptorSetLayoutBindingsLookup)
{
    this->descriptorSetLayoutBindingsLookup = std::move(newDescriptorSetLayoutBindingsLookup);
}

void Pipeline::setUniformBufferSize(uint8_t descriptorSetIndex, uint8_t descriptorSetBindingIndex, size_t size)
{
    auto &sizes = this->descriptorSetLayoutBindingSizesLookup[descriptorSetIndex];

    if (sizes.empty())
    {
        sizes = {};
    }

    sizes[descriptorSetBindingIndex] = size;
}

const vk::UniquePipeline &Pipeline::getVulkanPipeline() const
{
    return vulkanPipeline;
}

const vk::UniquePipelineLayout &Pipeline::getPipelineLayout() const
{
    return pipelineLayout;
}

void Pipeline::setDescriptorSetVisibilities(std::vector<DescriptorSetVisibility> &&newDescriptorSetVisibilities)
{
    this->descriptorSetVisibilities = newDescriptorSetVisibilities;
}

} // namespace pvk
