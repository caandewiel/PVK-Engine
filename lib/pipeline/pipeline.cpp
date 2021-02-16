//
//  pipeline.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 21/01/2021.
//

#include "pipeline.hpp"

namespace pvk {
    Pipeline::~Pipeline() {}
    
    void Pipeline::registerObject(std::shared_ptr<Object> object) {
        this->objects.emplace_back(std::move(object));
    }
    
    void Pipeline::registerTexture(Texture *texture, uint32_t binding) {
        this->textures[binding] = texture;
    }

    void Pipeline::prepare() {
        uint32_t numberOfSwapchainImages = (uint32_t) Context::getSwapchainImages().size();
        uint32_t numberOfUniformBuffers = 0;
        uint32_t numberOfCombinedImageSamplers = 0;
        uint32_t numberOfNodes = 0;

        // First count the amount of resources necessary for the descriptor pool
        getNumberOfResources(numberOfUniformBuffers, numberOfCombinedImageSamplers);

        for (auto &object : this->objects) {
            numberOfNodes += (uint32_t) object->gltfObject->nodeLookup.size();
        }

        // Create the actual descriptor pool for this pipeline
        std::vector<vk::DescriptorPoolSize> poolSizes{
                {
                        vk::DescriptorType::eUniformBuffer,
                        numberOfNodes * numberOfUniformBuffers * numberOfSwapchainImages
                },
                {
                        vk::DescriptorType::eCombinedImageSampler,
                        numberOfNodes * numberOfCombinedImageSamplers * numberOfSwapchainImages
                },
        };

        // @TODO: Allow multiple descriptor pools per descriptor set visibility
        this->descriptorPool = Context::getLogicalDevice().createDescriptorPoolUnique({{}, numberOfNodes * numberOfSwapchainImages, (uint32_t) poolSizes.size(), poolSizes.data()});

        for (auto &object : this->objects) {
            // @TODO: Do not hardcode the descriptor set layout
            object->gltfObject->initializeWriteDescriptorSets(Context::getLogicalDevice(),
                                                              this->descriptorPool.get(),
                                                              this->descriptorSetLayouts[0].get(),
                                                              numberOfSwapchainImages);
        }

        std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {};

        for (auto &object : this->objects) {
            for (auto &node : object->gltfObject->nodeLookup) {
                writeDescriptorSets = initializeDescriptorSet(numberOfSwapchainImages, writeDescriptorSets, *node.second);
            }
        }

        Context::getLogicalDevice().updateDescriptorSets(writeDescriptorSets, nullptr);
    }

    void Pipeline::getNumberOfResources(uint32_t &numberOfUniformBuffers, uint32_t &numberOfCombinedImageSamplers) {
        for (auto &descriptorSetLayoutBindings : descriptorSetLayoutBindingsLookup) {
            for (auto &descriptor : descriptorSetLayoutBindings) {
                if (descriptor.descriptorType == vk::DescriptorType::eUniformBuffer) {
                    numberOfUniformBuffers++;
                } else if (descriptor.descriptorType == vk::DescriptorType::eCombinedImageSampler) {
                    numberOfCombinedImageSamplers++;
                } else {
                    throw std::runtime_error("Unsupported descriptor type");
                }
            }
        }
    }

    std::vector<vk::WriteDescriptorSet> &Pipeline::initializeDescriptorSet(uint32_t numberOfSwapchainImages,
                                                                           std::vector<vk::WriteDescriptorSet> &writeDescriptorSets,
                                                                           gltf::Node &node) {
        for (uint32_t i = 0; i < numberOfSwapchainImages; i++) {
            for (auto &descriptorSetLayoutBindings : descriptorSetLayoutBindingsLookup) {
                for (auto &descriptor : descriptorSetLayoutBindings) {
                    switch (descriptor.descriptorType) {
                        case vk::DescriptorType::eUniformBuffer: {
                            addWriteDescriptorSetUniformBuffer(writeDescriptorSets, node, descriptor, i);
                            break;
                        }
                        case vk::DescriptorType::eCombinedImageSampler: {
                            addWriteDescriptorSetCombinedImageSampler(writeDescriptorSets, node, descriptor, i);
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
                                                      vk::DescriptorSetLayoutBinding &descriptor,
                                                      uint32_t i)
    {
        auto numberOfSwapchainImages = (uint32_t) Context::getSwapchainImages().size();

        node.uniformBuffers[descriptor.binding].resize(numberOfSwapchainImages);
        node.uniformBuffersMemory[descriptor.binding].resize(numberOfSwapchainImages);
        node.descriptorBuffersInfo[descriptor.binding].resize(numberOfSwapchainImages);

        pvk::buffer::create(this->descriptorSetLayoutBindingSizesLookup[0][descriptor.binding],
                            vk::BufferUsageFlagBits::eUniformBuffer,
                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                            node.uniformBuffers[descriptor.binding][i],
                            node.uniformBuffersMemory[descriptor.binding][i]);
        
        node.descriptorBuffersInfo[descriptor.binding][i].buffer   = node.uniformBuffers[descriptor.binding][i];
        node.descriptorBuffersInfo[descriptor.binding][i].offset   = 0;
        node.descriptorBuffersInfo[descriptor.binding][i].range    = this->descriptorSetLayoutBindingSizesLookup[0][descriptor.binding];

        writeDescriptorSets.emplace_back(node.descriptorSets[i].get(), descriptor.binding, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &node.descriptorBuffersInfo[descriptor.binding][i]);
    }
    
    void Pipeline::addWriteDescriptorSetCombinedImageSampler(std::vector<vk::WriteDescriptorSet> &writeDescriptorSets,
                                                             const gltf::Node &node,
                                                             const vk::DescriptorSetLayoutBinding &descriptor,
                                                             uint32_t i)
    {
        writeDescriptorSets.emplace_back(node.descriptorSets[i].get(), descriptor.binding, 0, 1, vk::DescriptorType::eCombinedImageSampler, this->textures[descriptor.binding]->getDescriptorImageInfo());
    }

    void Pipeline::setDescriptorSetLayouts(std::vector<vk::UniqueDescriptorSetLayout> &newDescriptorSetLayouts) {
        this->descriptorSetLayouts = std::move(newDescriptorSetLayouts);
    }

    void Pipeline::setDescriptorSetLayoutBindingsLookup(
            const std::vector<std::vector<vk::DescriptorSetLayoutBinding>> &newDescriptorSetLayoutBindingsLookup) {
        Pipeline::descriptorSetLayoutBindingsLookup = newDescriptorSetLayoutBindingsLookup;
    }

    void Pipeline::setUniformBufferSize(uint8_t descriptorSetIndex, uint8_t descriptorSetBindingIndex, size_t size) {
        auto &sizes = this->descriptorSetLayoutBindingSizesLookup[descriptorSetIndex];

        if (sizes.empty()) { sizes = {}; }

        sizes[descriptorSetBindingIndex] = size;
    }

    auto Pipeline::getVulkanPipeline() const -> const vk::UniquePipeline & {
        return vulkanPipeline;
    }

    auto Pipeline::getPipelineLayout() const -> const vk::UniquePipelineLayout & {
        return pipelineLayout;
    }
}
