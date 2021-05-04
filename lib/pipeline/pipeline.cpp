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
    // Make sure we destroy the objects and textures before the rest of the pipeline.
    for (auto &object : this->objects)
    {
        object.reset();
    }

//    for (auto &textureByDescriptorSet : this->textures)
//    {
//        for (auto &texture : textureByDescriptorSet)
//        {
//            texture.second.reset();
//        }
//    }
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
    initializeDescriptorPools();

    for (auto &object : this->objects)
    {
        initializeDescriptorSets(object);
    }

    Context::getLogicalDevice().updateDescriptorSets(getWriteDescriptorSets(), nullptr);
}

void Pipeline::initializeDescriptorSets(std::shared_ptr<Object> &object)
{
    auto numberOfSwapChainImages = static_cast<uint32_t>(Context::getNumberOfSwapChainImages());

    for (size_t i = 0; i < this->descriptorSetLayouts.size(); i++)
    {
        auto &descriptorSetLayout = this->descriptorSetLayouts[i];
        auto &visibility = this->descriptorSetVisibilities.at(i);
        std::vector<vk::DescriptorSetLayout> layouts(numberOfSwapChainImages, descriptorSetLayout.get());

        switch (visibility)
        {
        case DescriptorSetVisibility::NODE: {
            for (const auto &node : object->gltfObject->getNodes())
            {
                node.second->getDescriptorSets()[i].resize(descriptorSetLayouts.size());
                vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo = {
                    this->descriptorPools[i].get(), numberOfSwapChainImages, layouts.data()};
                node.second->initializeDescriptorSets(descriptorSetAllocateInfo, i);
            }
            break;
        }
        case DescriptorSetVisibility::PRIMITIVE: {
            for (auto &primitivesByNode : object->gltfObject->primitiveLookup)
            {
                for (auto &primitive : primitivesByNode.second)
                {
                    primitive.lock()->getDescriptorSets()[i].resize(descriptorSetLayouts.size());
                    vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo = {
                        this->descriptorPools[i].get(), numberOfSwapChainImages, layouts.data()};
                    primitive.lock()->initializeDescriptorSets(descriptorSetAllocateInfo, i);
                }
            }
            break;
        }
        case DescriptorSetVisibility::OBJECT: {
            throw std::runtime_error("Needs to be implemented.");
        }
        }
    }
}

std::vector<vk::WriteDescriptorSet> Pipeline::getWriteDescriptorSets()
{
    std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {};

    for (auto &object : objects)
    {
        for (const auto &node : object->gltfObject->getNodes())
        {
            writeDescriptorSets = initializeDescriptorSet(writeDescriptorSets, *node.second);
        }

        for (auto &primitivesByNode : object->gltfObject->primitiveLookup) {
            for (auto &primitive : primitivesByNode.second) {
                writeDescriptorSets = initializeDescriptorSet(writeDescriptorSets, *primitive.lock());
            }
        }
    }

    return writeDescriptorSets;
}

uint32_t Pipeline::getNumberOfNodes()
{
    auto numberOfNodes = 0;

    for (auto &object : objects)
    {
        numberOfNodes += static_cast<uint32_t>(object->gltfObject->getNodes().size());
    }

    return numberOfNodes;
}

uint32_t Pipeline::getNumberOfPrimitives()
{
    auto numberOfPrimitives = 0;

    for (auto &object : objects)
    {
        numberOfPrimitives += static_cast<uint32_t>(object->gltfObject->getNumberOfPrimitives());
    }

    return numberOfPrimitives;
}

void Pipeline::initializeDescriptorPools()
{
    auto numberOfNodes = this->getNumberOfNodes();
    auto numberOfPrimitives = this->getNumberOfPrimitives();

    for (size_t i = 0; i < this->descriptorSetLayouts.size(); i++)
    {
        uint32_t numberOfInstances = 0;

        switch (this->descriptorSetVisibilities[i])
        {
        case DescriptorSetVisibility::NODE:
            numberOfInstances = numberOfNodes;
            break;
        case DescriptorSetVisibility::PRIMITIVE:
            numberOfInstances = numberOfPrimitives;
            break;
        case DescriptorSetVisibility::OBJECT:
            throw std::runtime_error("Not yet implemented");
        default:
            break;
        }

        std::vector<vk::DescriptorPoolSize> poolSizes{
            {vk::DescriptorType::eUniformBuffer,
             numberOfInstances * getNumberOfResources<vk::DescriptorType::eUniformBuffer>(i)},
            {vk::DescriptorType::eCombinedImageSampler,
             numberOfInstances * getNumberOfResources<vk::DescriptorType::eCombinedImageSampler>(i)},
        };

        auto descriptorPool = Context::getLogicalDevice().createDescriptorPoolUnique(
            {{vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet},
             static_cast<uint32_t>(numberOfInstances * Context::getNumberOfSwapChainImages()),
             static_cast<uint32_t>(poolSizes.size()),
             poolSizes.data()});

        descriptorPools.emplace_back(std::move(descriptorPool));
    }
}

template <vk::DescriptorType T> uint32_t Pipeline::getNumberOfResources(uint32_t descriptorSetLayoutIndex)
{
    auto numberOfResources = 0;

    for (auto &descriptor : descriptorSetLayoutBindingsLookup.at(descriptorSetLayoutIndex))
    {
        if (descriptor.descriptorType == T)
        {
            numberOfResources++;
        }
    }

    return numberOfResources;
}

const std::vector<vk::WriteDescriptorSet> &Pipeline::initializeDescriptorSet(
    std::vector<vk::WriteDescriptorSet> &writeDescriptorSets,
    Drawable &drawable)
{
    for (uint32_t i = 0; i < Context::getNumberOfSwapChainImages(); i++)
    {
        for (size_t j = 0; j < descriptorSetLayoutBindingsLookup.size(); j++)
        {
            if (drawable.getType() == DrawableType::DRAWABLE_NODE && this->descriptorSetVisibilities[j] != DescriptorSetVisibility::NODE) {
                continue;
            } else if (drawable.getType() == DrawableType::DRAWABLE_PRIMITIVE && this->descriptorSetVisibilities[j] != DescriptorSetVisibility::PRIMITIVE) {
                continue;
            }

            const auto &descriptorSetLayoutBindings = descriptorSetLayoutBindingsLookup[j];

            for (const auto &descriptor : descriptorSetLayoutBindings)
            {
                switch (descriptor.descriptorType)
                {
                case vk::DescriptorType::eUniformBuffer: {
                    addWriteDescriptorSetUniformBuffer(writeDescriptorSets, drawable, descriptor, j, i);
                    break;
                }
                case vk::DescriptorType::eCombinedImageSampler: {
                    addWriteDescriptorSetCombinedImageSampler(writeDescriptorSets, drawable, descriptor, j, i);
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
                                                  Drawable &drawable,
                                                  const vk::DescriptorSetLayoutBinding &descriptor,
                                                  uint32_t descriptorSetIndex,
                                                  uint32_t swapChainImageIndex) const
{
    auto bindingSize = this->getBindingSize(descriptorSetIndex, descriptor.binding);
    drawable.addUniformBufferToDescriptorSet(descriptor, bindingSize, descriptorSetIndex, swapChainImageIndex);

    auto &uniformBuffer = drawable.getUniformBuffer(descriptorSetIndex, descriptor.binding, swapChainImageIndex).get();

    drawable.setDescriptorBufferInfo(
        {uniformBuffer, 0, bindingSize}, descriptorSetIndex, descriptor.binding, swapChainImageIndex);

    writeDescriptorSets.emplace_back(
        drawable.getDescriptorSet(descriptorSetIndex, swapChainImageIndex).get(),
        descriptor.binding,
        0,
        1,
        vk::DescriptorType::eUniformBuffer,
        nullptr,
        &drawable.getDescriptorBufferInfo(descriptorSetIndex, descriptor.binding, swapChainImageIndex));
}

void Pipeline::addWriteDescriptorSetCombinedImageSampler(std::vector<vk::WriteDescriptorSet> &writeDescriptorSets,
                                                         const Drawable &drawable,
                                                         const vk::DescriptorSetLayoutBinding &descriptor,
                                                         uint32_t descriptorSetIndex,
                                                         uint32_t swapChainImageIndex)
{
    writeDescriptorSets.emplace_back(
        drawable.getDescriptorSet(descriptorSetIndex, swapChainImageIndex).get(),
        descriptor.binding,
        0,
        1,
        vk::DescriptorType::eCombinedImageSampler,
        (drawable.getType() == DRAWABLE_NODE) ? this->textures[descriptorSetIndex][descriptor.binding].lock()->getDescriptorImageInfo() : dynamic_cast<const gltf::Primitive &>(drawable).material->getTextureByBinding(descriptor.binding).getDescriptorImageInfo());
}

void Pipeline::setDescriptorSetLayouts(std::vector<vk::UniqueDescriptorSetLayout> &&newDescriptorSetLayouts)
{
    this->descriptorSetLayouts = std::move(newDescriptorSetLayouts);
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
