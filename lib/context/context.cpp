#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "cppcoreguidelines-avoid-non-const-global-variables"
//
//  context.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 20/01/2021.
//

#include "context.hpp"

namespace pvk
{
static vk::PhysicalDevice physicalDevice = nullptr;
static vk::UniqueInstance instance{nullptr};
static vk::UniqueDevice logicalDevice{nullptr};
static vk::UniqueCommandPool commandPool{nullptr};
static vk::UniquePipelineCache pipelineCache{nullptr};
static vk::Queue graphicsQueue{nullptr};
static std::vector<vk::Image> swapChainImages;

void Context::tearDown()
{
    pipelineCache.reset();
    commandPool.reset();
    logicalDevice.reset();
    instance.reset();
    graphicsQueue = nullptr;
    physicalDevice = nullptr;
}

void Context::setPhysicalDevice(vk::PhysicalDevice &&_physicalDevice)
{
    physicalDevice = _physicalDevice;
}

void Context::setLogicalDevice(vk::UniqueDevice &&_logicalDevice)
{
    logicalDevice = std::move(_logicalDevice);
}

void Context::setCommandPool(vk::UniqueCommandPool &&_commandPool)
{
    commandPool = std::move(_commandPool);
}

void Context::setPipelineCache(vk::UniquePipelineCache &&_pipelineCache)
{
    pipelineCache = std::move(_pipelineCache);
}

void Context::setInstance(vk::UniqueInstance &&_instance)
{
    instance = std::move(_instance);
}

void Context::setGraphicsQueue(vk::Queue &&_queue) {
    graphicsQueue = _queue;
}

void Context::setSwapChainImages(std::vector<vk::Image> _swapChainImages)
{
    swapChainImages = std::move(_swapChainImages);
}

vk::PhysicalDevice Context::getPhysicalDevice()
{
    return physicalDevice;
}

vk::Device Context::getLogicalDevice()
{
    return logicalDevice.get();
}

vk::CommandPool Context::getCommandPool()
{
    return commandPool.get();
}

vk::PipelineCache Context::getPipelineCache()
{
    return pipelineCache.get();
}

vk::Instance Context::getInstance()
{
    return instance.get();
}

vk::Queue Context::getGraphicsQueue()
{
    return graphicsQueue;
}

const std::vector<vk::Image> &Context::getSwapChainImages()
{
    return swapChainImages;
}

size_t Context::getNumberOfSwapChainImages()
{
    return swapChainImages.size();
}
} // namespace pvk

#pragma clang diagnostic pop