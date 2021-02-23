//
// Created by Christian aan de Wiel on 11/02/2021.
//

#ifndef PVK_PIPELINEPARSER_HPP
#define PVK_PIPELINEPARSER_HPP

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "../context/context.hpp"
#include "json.hpp"
#include "pipeline.hpp"

using json = nlohmann::json;

namespace pvk
{
static const std::map<std::string, vk::DescriptorType> descriptorTypeMapping = {
    {"UNIFORM_BUFFER", vk::DescriptorType::eUniformBuffer},
    {"COMBINED_IMAGE_SAMPLER", vk::DescriptorType::eCombinedImageSampler},
};

static const std::map<std::string, vk::ShaderStageFlags> shaderStageMapping = {
    {"VERTEX_AND_FRAGMENT", vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
    {"FRAGMENT", vk::ShaderStageFlagBits::eFragment},
    {"VERTEX", vk::ShaderStageFlagBits::eVertex},
};

static const std::map<std::string, Pipeline::DescriptorSetVisibility> visibilityMapping = {
    {"OBJECT", Pipeline::DescriptorSetVisibility::OBJECT},
    {"NODE", Pipeline::DescriptorSetVisibility::NODE},
    {"PRIMITIVE", Pipeline::DescriptorSetVisibility::PRIMITIVE},
};

static const std::map<std::string, vk::CullModeFlags> cullModeMapping = {
    {"BACK", vk::CullModeFlagBits::eBack},
    {"FRONT", vk::CullModeFlagBits::eFront},
};

struct DescriptorBinding
{
    DescriptorBinding(std::string name,
                      uint8_t index,
                      vk::DescriptorType descriptorType,
                      vk::ShaderStageFlags shaderStageFlags)
        : name(name), index(index), descriptorType(descriptorType), shaderStageFlags(shaderStageFlags) {};
    std::string name;
    uint8_t index;
    vk::DescriptorType descriptorType;
    vk::ShaderStageFlags shaderStageFlags;
};

struct DescriptorSet
{
    std::vector<std::unique_ptr<DescriptorBinding>> bindings{};
    Pipeline::DescriptorSetVisibility visibility{};
};

constexpr char FIELD_DESCRIPTOR_SETS[] = "descriptorSets";
constexpr char FIELD_VISIBILITY[] = "visibility";
constexpr char FIELD_BINDINGS[] = "bindings";
constexpr char FIELD_NAME[] = "name";
constexpr char FIELD_BINDING_INDEX[] = "bindingIndex";
constexpr char FIELD_TYPE[] = "type";
constexpr char FIELD_STAGE[] = "stage";
constexpr char FIELD_CULLING_MODE[] = "cullingMode";

std::unique_ptr<pvk::Pipeline> createPipelineFromDefinition(const std::string &filePath,
                                                            vk::RenderPass &renderPass,
                                                            vk::Extent2D &swapChainExtent)
{
    // Load the pipeline definition and parser the JSON content
    std::ifstream input(filePath);
    json jsonContent;
    input >> jsonContent;

    // Parse the different descriptor sets described in the definition file
    auto &descriptorSets = jsonContent[FIELD_DESCRIPTOR_SETS];

    std::vector<std::unique_ptr<DescriptorSet>> _descriptorSets;

    for (auto &descriptorSet : descriptorSets)
    {
        auto _descriptorSet = std::make_unique<DescriptorSet>();
        _descriptorSet->visibility = visibilityMapping.at(descriptorSet[FIELD_VISIBILITY].get<std::string>());

        for (auto &binding : descriptorSet[FIELD_BINDINGS])
        {
            auto _binding =
                std::make_unique<DescriptorBinding>(binding[FIELD_NAME].get<std::string>(),
                                                    binding[FIELD_BINDING_INDEX].get<std::uint8_t>(),
                                                    descriptorTypeMapping.at(binding[FIELD_TYPE].get<std::string>()),
                                                    shaderStageMapping.at(binding[FIELD_STAGE].get<std::string>()));

            _descriptorSet->bindings.emplace_back(std::move(_binding));
        }

        _descriptorSets.emplace_back(std::move(_descriptorSet));
    }

    // Create native Vulkan descriptor set layouts for all defined descriptor sets
    std::vector<vk::UniqueDescriptorSetLayout> descriptorSetLayouts{};
    std::vector<Pipeline::DescriptorSetVisibility> descriptorSetVisibilities;
    std::vector<std::vector<vk::DescriptorSetLayoutBinding>> descriptorSetLayoutBindingsLookup;

    for (auto &_descriptorSet : _descriptorSets)
    {
        // First create all descriptor set layout bindings
        std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings;
        descriptorSetLayoutBindings.reserve(_descriptorSet->bindings.size());

        for (auto &_descriptorBinding : _descriptorSet->bindings)
        {
            descriptorSetLayoutBindings.push_back({
                _descriptorBinding->index,
                _descriptorBinding->descriptorType,
                1,
                _descriptorBinding->shaderStageFlags,
            });
        }
        descriptorSetLayoutBindingsLookup.push_back(descriptorSetLayoutBindings);

        // Now create the actual descriptor set layout
        vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
        descriptorSetLayoutCreateInfo.setBindings(descriptorSetLayoutBindings);

        descriptorSetLayouts.emplace_back(
            Context::getLogicalDevice().createDescriptorSetLayoutUnique(descriptorSetLayoutCreateInfo));
        descriptorSetVisibilities.emplace_back(_descriptorSet->visibility);
    }

    // Create the pipeline layout
    vk::PipelineLayoutCreateInfo pipelineCreateInfo;
    auto rawDescriptorSetLayouts = vk::uniqueToRaw(descriptorSetLayouts);
    pipelineCreateInfo.setSetLayouts(rawDescriptorSetLayouts);
    auto pipelineLayout = Context::getLogicalDevice().createPipelineLayoutUnique(pipelineCreateInfo);

    pvk::pipeline::Builder pipelineBuilder{renderPass, std::move(pipelineLayout)};

    auto bindingDescriptions = pvk::Vertex::getBindingDescription();
    auto attributeDescriptions = pvk::Vertex::getAttributeDescriptions();
    pipelineBuilder.bindingDescriptions = bindingDescriptions;
    pipelineBuilder.attributeDescriptions = attributeDescriptions;
    std::vector<vk::Viewport> viewports = {
        {0.0f, 0.0f, (float)swapChainExtent.width, (float)swapChainExtent.height, 0.0f, 1.0f}};
    std::vector<vk::Rect2D> scissors = {{vk::Offset2D(0, 0), swapChainExtent}};
    pipelineBuilder.viewports = viewports;
    pipelineBuilder.scissors = scissors;

    pipelineBuilder.rasterizationState.cullMode =
        cullModeMapping.at(jsonContent[FIELD_CULLING_MODE].get<std::string>());
    pipelineBuilder.rasterizationState.frontFace = vk::FrontFace::eCounterClockwise;

    if (jsonContent["enableDepth"].get<bool>())
    {
        pipelineBuilder.depthStencilState.depthWriteEnable = true;
        pipelineBuilder.depthStencilState.depthTestEnable = true;
    }
    else
    {
        pipelineBuilder.depthStencilState.depthWriteEnable = false;
        pipelineBuilder.depthStencilState.depthTestEnable = false;
    }

    // Load vertex and fragment shaders
    auto vertexShaderContent = pvk::util::readFile(jsonContent["vertexShader"].get<std::string>());
    auto fragmentShaderContent = pvk::util::readFile(jsonContent["fragmentShader"].get<std::string>());

    vk::UniqueShaderModule vertexShader = pvk::Context::getLogicalDevice().createShaderModuleUnique(
        {vk::ShaderModuleCreateFlags(),
         vertexShaderContent.size(),
         reinterpret_cast<const uint32_t *>(vertexShaderContent.data())});
    vk::UniqueShaderModule fragmentShader = pvk::Context::getLogicalDevice().createShaderModuleUnique(
        {vk::ShaderModuleCreateFlags(),
         fragmentShaderContent.size(),
         reinterpret_cast<const uint32_t *>(fragmentShaderContent.data())});

    vk::PipelineShaderStageCreateInfo vertexShaderStage;
    vertexShaderStage.setFlags(vk::PipelineShaderStageCreateFlags());
    vertexShaderStage.setStage(vk::ShaderStageFlagBits::eVertex);
    vertexShaderStage.setModule(vertexShader.get());
    vertexShaderStage.setPName("main");

    vk::PipelineShaderStageCreateInfo fragmentShaderStage;
    fragmentShaderStage.setFlags(vk::PipelineShaderStageCreateFlags());
    fragmentShaderStage.setStage(vk::ShaderStageFlagBits::eFragment);
    fragmentShaderStage.setModule(fragmentShader.get());
    fragmentShaderStage.setPName("main");

    pipelineBuilder.shaderStages.push_back(vertexShaderStage);
    pipelineBuilder.shaderStages.push_back(fragmentShaderStage);

    // Now create the actual pipeline
    auto pipeline =
        std::make_unique<Pipeline>(pipelineBuilder.create(Context::getPipelineCache()), std::move(pipelineLayout));

    pipeline->setDescriptorSetLayouts(std::move(descriptorSetLayouts));
    pipeline->setDescriptorSetLayoutBindingsLookup(std::move(descriptorSetLayoutBindingsLookup));
    pipeline->setDescriptorSetVisibilities(std::move(descriptorSetVisibilities));

    return pipeline;
}

} // namespace pvk

#endif // PVK_PIPELINEPARSER_HPP
