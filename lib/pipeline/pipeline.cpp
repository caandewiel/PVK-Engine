//
//  pipeline.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 21/01/2021.
//

#include "pipeline.hpp"

namespace pvk {
    Pipeline::Pipeline(vk::RenderPass &renderPass,
                       vk::Extent2D &swapChainExtent,
                       pvk::Shader *shader,
                       vk::CullModeFlags cullModeFlags,
                       bool enableDepth)
    {
        this->renderPass = renderPass;
        this->shader = shader;
        
        this->swapChainExtent = swapChainExtent;
        
        auto layoutBindings = shader->getDescriptorSetLayoutBindings();
        vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {};
        descriptorSetLayoutCreateInfo.setBindings(layoutBindings);
        this->descriptorSetLayout = Context::getLogicalDevice().createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
        
        vk::PipelineLayoutCreateInfo pipelineCreateInfo {};
        pipelineCreateInfo.setSetLayouts(this->descriptorSetLayout);
        pipelineLayout = Context::getLogicalDevice().createPipelineLayout(pipelineCreateInfo);

        pvk::pipeline::Builder pipelineBuilder {renderPass, pipelineLayout};

        auto bindingDescriptions                = pvk::Vertex::getBindingDescription();
        auto attributeDescriptions              = pvk::Vertex::getAttributeDescriptions();
        pipelineBuilder.bindingDescriptions     = bindingDescriptions;
        pipelineBuilder.attributeDescriptions   = attributeDescriptions;
        std::vector<vk::Viewport> viewports     = {{0.0f, 0.0f, (float) swapChainExtent.width, (float) swapChainExtent.height, 0.0f, 1.0f}};
        std::vector<vk::Rect2D> scissors        = {{vk::Offset2D(0, 0), swapChainExtent}};
        pipelineBuilder.viewports               = viewports;
        pipelineBuilder.scissors                = scissors;

        pipelineBuilder.rasterizationState.cullMode = cullModeFlags;
        pipelineBuilder.rasterizationState.frontFace = vk::FrontFace::eCounterClockwise;
        
        if (enableDepth) {
            pipelineBuilder.depthStencilState.depthWriteEnable = true;
            pipelineBuilder.depthStencilState.depthTestEnable = true;
        } else {
            pipelineBuilder.depthStencilState.depthWriteEnable = false;
            pipelineBuilder.depthStencilState.depthTestEnable = false;
        }

        vk::PipelineShaderStageCreateInfo vertexShaderStage;
        vertexShaderStage.setFlags(vk::PipelineShaderStageCreateFlags());
        vertexShaderStage.setStage(vk::ShaderStageFlagBits::eVertex);
        vertexShaderStage.setModule(shader->getVertexShaderModule());
        vertexShaderStage.setPName("main");

        vk::PipelineShaderStageCreateInfo fragmentShaderStage;
        fragmentShaderStage.setFlags(vk::PipelineShaderStageCreateFlags());
        fragmentShaderStage.setStage(vk::ShaderStageFlagBits::eFragment);
        fragmentShaderStage.setModule(shader->getFragmentShaderModule());
        fragmentShaderStage.setPName("main");

        pipelineBuilder.shaderStages.push_back(vertexShaderStage);
        pipelineBuilder.shaderStages.push_back(fragmentShaderStage);
        
        this->vulkanPipeline = pipelineBuilder.create(Context::getPipelineCache());
        
        pipelineBuilder.clearShaderModules();
    }
    
    Pipeline::~Pipeline() {
        Context::getLogicalDevice().destroyDescriptorSetLayout(this->descriptorSetLayout, nullptr);
        Context::getLogicalDevice().destroyDescriptorPool(this->descriptorPool, nullptr);
        Context::getLogicalDevice().destroyPipelineLayout(this->pipelineLayout, nullptr);
        Context::getLogicalDevice().destroyPipeline(this->vulkanPipeline, nullptr);
    }
    
    void Pipeline::registerObject(Object* object) {
        this->objects.push_back(object);
    }
    
    void Pipeline::registerTexture(Texture *texture, uint32_t binding) {
        this->textures[binding] = texture;
    }
    
    void Pipeline::prepareForRenderStage() {
        uint32_t numberOfSwapchainImages = (uint32_t) Context::getSwapchainImages().size();
        uint32_t numberOfUniformBuffers = 0;
        uint32_t numberOfCombinedImageSamplers = 0;
        uint32_t numberOfNodes = 0;
        
        auto descriptorSetLayoutBindings = shader->getDescriptorSetLayoutBindings();
        
        for (auto &descriptor : descriptorSetLayoutBindings) {
            if (descriptor.descriptorType == vk::DescriptorType::eUniformBuffer) {
                numberOfUniformBuffers++;
            } else if (descriptor.descriptorType == vk::DescriptorType::eCombinedImageSampler) {
                numberOfCombinedImageSamplers++;
            } else {
                throw std::runtime_error("Unsupported descriptor type");
            }
        }
        
        for (auto &object : this->objects) {
            numberOfNodes += (uint32_t) object->gltfObject->nodeLookup.size();
        }

        std::vector<vk::DescriptorPoolSize> poolSizes{
            {vk::DescriptorType::eUniformBuffer, numberOfNodes * numberOfUniformBuffers * numberOfSwapchainImages},
            {vk::DescriptorType::eCombinedImageSampler, numberOfNodes * numberOfCombinedImageSamplers * numberOfSwapchainImages},
        };

        this->descriptorPool = Context::getLogicalDevice().createDescriptorPool({{}, numberOfNodes * numberOfSwapchainImages, (uint32_t) poolSizes.size(), poolSizes.data()});
        
        for (auto &object : this->objects) {
            object->gltfObject->initializeDescriptorSets(Context::getLogicalDevice(),
                                                         this->descriptorPool,
                                                         this->descriptorSetLayout,
                                                         numberOfSwapchainImages);
        }

        std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {};
        
        for (auto &object : this->objects) {
            for (auto &node : object->gltfObject->nodeLookup) {
                for (uint32_t i = 0; i < numberOfSwapchainImages; i++) {
                    for (auto &descriptor : shader->getDescriptorSetLayoutBindings()) {
                        switch (descriptor.descriptorType) {
                            case vk::DescriptorType::eUniformBuffer:
                                this->addWriteDescriptorSetUniformBuffer(writeDescriptorSets, node.second, descriptor, i);
                                break;
                                
                            case vk::DescriptorType::eCombinedImageSampler:
                                this->addWriteDescriptorSetCombinedImageSampler(writeDescriptorSets, node.second, descriptor, i);
                                break;
                                
                            default:
                                throw std::runtime_error("Unsupported descriptor type");
                        }
                    }
                }
            }
        }

        Context::getLogicalDevice().updateDescriptorSets(writeDescriptorSets, nullptr);
    }
    
    void Pipeline::addWriteDescriptorSetUniformBuffer(std::vector<vk::WriteDescriptorSet> &writeDescriptorSets,
                                                      pvk::gltf::Node *node,
                                                      vk::DescriptorSetLayoutBinding &descriptor,
                                                      uint32_t i)
    {
        auto numberOfSwapchainImages = (uint32_t) Context::getSwapchainImages().size();
        
        node->uniformBuffers[descriptor.binding].resize(numberOfSwapchainImages);
        node->uniformBuffersMemory[descriptor.binding].resize(numberOfSwapchainImages);
        node->descriptorBuffersInfo[descriptor.binding].resize(numberOfSwapchainImages);
        
        pvk::buffer::create(shader->getSizeForBinding(descriptor.binding),
                            vk::BufferUsageFlagBits::eUniformBuffer,
                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                            node->uniformBuffers[descriptor.binding][i],
                            node->uniformBuffersMemory[descriptor.binding][i]);
        
        node->descriptorBuffersInfo[descriptor.binding][i].buffer   = node->uniformBuffers[descriptor.binding][i];
        node->descriptorBuffersInfo[descriptor.binding][i].offset   = 0;
        node->descriptorBuffersInfo[descriptor.binding][i].range    = shader->getSizeForBinding(descriptor.binding);

        writeDescriptorSets.reserve(writeDescriptorSets.size() + 1);
        writeDescriptorSets.emplace_back(node->descriptorSets[i].get(), descriptor.binding, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &node->descriptorBuffersInfo[descriptor.binding][i]);
    }
    
    void Pipeline::addWriteDescriptorSetCombinedImageSampler(std::vector<vk::WriteDescriptorSet> &writeDescriptorSets,
                                                             pvk::gltf::Node *node,
                                                             vk::DescriptorSetLayoutBinding &descriptor,
                                                             uint32_t i)
    {
        writeDescriptorSets.reserve(writeDescriptorSets.size() + 1);
        writeDescriptorSets.emplace_back(node->descriptorSets[i].get(), descriptor.binding, 0, 1, vk::DescriptorType::eCombinedImageSampler, this->textures[descriptor.binding]->getDescriptorImageInfo());
    }
}
