//
// Created by Christian aan de Wiel on 11/02/2021.
//

#ifndef PVK_PIPELINEPARSER_HPP
#define PVK_PIPELINEPARSER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <vulkan/vulkan.hpp>

#include "json.hpp"
#include "pipeline.hpp"
#include "../context/context.hpp"

using json = nlohmann::json;

namespace pvk {
    enum DescriptorSetVisibility {
        OBJECT, NODE, PRIMITIVE
    };

    static std::map<std::string, vk::DescriptorType> descriptorTypeMapping = {
            {"UNIFORM_BUFFER",         vk::DescriptorType::eUniformBuffer},
            {"COMBINED_IMAGE_SAMPLER", vk::DescriptorType::eCombinedImageSampler},
    };

    static std::map<std::string, vk::ShaderStageFlags> shaderStageMapping = {
            {"VERTEX_AND_FRAGMENT", vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
            {"FRAGMENT",            vk::ShaderStageFlagBits::eFragment},
            {"VERTEX",              vk::ShaderStageFlagBits::eVertex},
    };

    static std::map<std::string, pvk::DescriptorSetVisibility> visibilityMapping = {
            {"OBJECT",    pvk::DescriptorSetVisibility::OBJECT},
            {"NODE",      pvk::DescriptorSetVisibility::NODE},
            {"PRIMITIVE", pvk::DescriptorSetVisibility::PRIMITIVE},
    };

    static std::map<std::string, vk::CullModeFlags> cullModeMapping = {
            {"BACK",  vk::CullModeFlagBits::eBack},
            {"FRONT", vk::CullModeFlagBits::eFront},
    };

    struct DescriptorBinding {
        std::string name;
        uint8_t index;
        vk::DescriptorType descriptorType;
        vk::ShaderStageFlags shaderStageFlags;
    };

    struct DescriptorSet {
        std::vector<DescriptorBinding *> bindings{};
        DescriptorSetVisibility visibility{};
    };

    pvk::Pipeline* createPipelineFromDefinition(const std::string &filePath,
                                                vk::RenderPass &renderPass,
                                                vk::Extent2D &swapChainExtent) {
        // Load the pipeline definition and parser the JSON content
        std::ifstream input(filePath);
        json jsonContent;
        input >> jsonContent;

        // Parse the different descriptor sets described in the definition file
        auto &descriptorSets = jsonContent["descriptorSets"];

        std::vector<DescriptorSet *> _descriptorSets;
        _descriptorSets.reserve(descriptorSets.size());

        for (auto &descriptorSet : descriptorSets) {
            auto _descriptorSet = new DescriptorSet();
            _descriptorSet->visibility = visibilityMapping[descriptorSet["visibility"].get<std::string>()];
            _descriptorSet->bindings.reserve(descriptorSet["bindings"].size());

            for (auto &binding : descriptorSet["bindings"]) {
                auto _binding = new DescriptorBinding{
                        binding["name"].get<std::string>(),
                        binding["bindingIndex"].get<std::uint8_t>(),
                        descriptorTypeMapping[binding["type"].get<std::string>()],
                        shaderStageMapping[binding["stage"].get<std::string>()],
                };

                _descriptorSet->bindings.emplace_back(_binding);
            }

            _descriptorSets.emplace_back(_descriptorSet);
        }

        // Create native Vulkan descriptor set layouts for all defined descriptor sets
        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
        descriptorSetLayouts.reserve(_descriptorSets.size());

        std::vector<std::vector<vk::DescriptorSetLayoutBinding>> descriptorSetLayoutBindingsLookup;

        for (auto &_descriptorSet : _descriptorSets) {
            // First create all descriptor set layout bindings
            std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings;
            descriptorSetLayoutBindings.reserve(_descriptorSet->bindings.size());

            for (auto &_descriptorBinding : _descriptorSet->bindings) {
                // @TODO: Do not hardcode the descriptor count
                descriptorSetLayoutBindings.push_back({_descriptorBinding->index,
                                                       _descriptorBinding->descriptorType,
                                                       1,
                                                       _descriptorBinding->shaderStageFlags,
                                                      });
            }
            descriptorSetLayoutBindingsLookup.push_back(descriptorSetLayoutBindings);

            // Now create the actual descriptor set layout
            vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
            descriptorSetLayoutCreateInfo.setBindings(descriptorSetLayoutBindings);

            vk::DescriptorSetLayout descriptorSetLayout = Context::getLogicalDevice().createDescriptorSetLayout(descriptorSetLayoutCreateInfo);

            descriptorSetLayouts.emplace_back(descriptorSetLayout);
        }

        // Create the pipeline layout
        vk::PipelineLayoutCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.setSetLayouts(descriptorSetLayouts);
        vk::PipelineLayout pipelineLayout = Context::getLogicalDevice().createPipelineLayout(pipelineCreateInfo);

        pvk::pipeline::Builder pipelineBuilder{renderPass, pipelineLayout};

        auto bindingDescriptions = pvk::Vertex::getBindingDescription();
        auto attributeDescriptions = pvk::Vertex::getAttributeDescriptions();
        pipelineBuilder.bindingDescriptions = bindingDescriptions;
        pipelineBuilder.attributeDescriptions = attributeDescriptions;
        std::vector<vk::Viewport> viewports = {{0.0f, 0.0f, (float) swapChainExtent.width, (float) swapChainExtent.height, 0.0f, 1.0f}};
        std::vector<vk::Rect2D> scissors = {{vk::Offset2D(0, 0), swapChainExtent}};
        pipelineBuilder.viewports = viewports;
        pipelineBuilder.scissors = scissors;

        pipelineBuilder.rasterizationState.cullMode = cullModeMapping[jsonContent["cullingMode"].get<std::string>()];
        pipelineBuilder.rasterizationState.frontFace = vk::FrontFace::eCounterClockwise;

        if (jsonContent["enableDepth"].get<bool>()) {
            pipelineBuilder.depthStencilState.depthWriteEnable = true;
            pipelineBuilder.depthStencilState.depthTestEnable = true;
        } else {
            pipelineBuilder.depthStencilState.depthWriteEnable = false;
            pipelineBuilder.depthStencilState.depthTestEnable = false;
        }

        // Load vertex and fragment shaders
        auto vertexShaderContent = pvk::util::readFile(jsonContent["vertexShader"].get<std::string>());
        auto fragmentShaderContent = pvk::util::readFile(jsonContent["fragmentShader"].get<std::string>());

        vk::ShaderModule vertexShader = pvk::Context::getLogicalDevice().createShaderModule(
                {
                        vk::ShaderModuleCreateFlags(),
                        vertexShaderContent.size(),
                        reinterpret_cast<const uint32_t *>(vertexShaderContent.data())
                }
        );
        vk::ShaderModule fragmentShader = pvk::Context::getLogicalDevice().createShaderModule(
                {
                        vk::ShaderModuleCreateFlags(),
                        fragmentShaderContent.size(),
                        reinterpret_cast<const uint32_t *>(fragmentShaderContent.data())
                }
        );

        vk::PipelineShaderStageCreateInfo vertexShaderStage;
        vertexShaderStage.setFlags(vk::PipelineShaderStageCreateFlags());
        vertexShaderStage.setStage(vk::ShaderStageFlagBits::eVertex);
        vertexShaderStage.setModule(vertexShader);
        vertexShaderStage.setPName("main");

        vk::PipelineShaderStageCreateInfo fragmentShaderStage;
        fragmentShaderStage.setFlags(vk::PipelineShaderStageCreateFlags());
        fragmentShaderStage.setStage(vk::ShaderStageFlagBits::eFragment);
        fragmentShaderStage.setModule(fragmentShader);
        fragmentShaderStage.setPName("main");

        pipelineBuilder.shaderStages.push_back(vertexShaderStage);
        pipelineBuilder.shaderStages.push_back(fragmentShaderStage);

        // Now create the actual pipeline
        auto pipeline = new pvk::Pipeline();
        pipeline->vulkanPipeline = pipelineBuilder.create(Context::getPipelineCache());
        pipeline->pipelineLayout = pipelineLayout;
        pipeline->setDescriptorSetLayouts(descriptorSetLayouts);
        pipeline->setDescriptorSetLayoutBindingsLookup(descriptorSetLayoutBindingsLookup);

        return pipeline;
    }

}

#endif //PVK_PIPELINEPARSER_HPP
