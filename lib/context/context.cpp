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

namespace pvk {
    static vk::PhysicalDevice physicalDevice = nullptr;
    static vk::UniqueInstance instance {nullptr};
    static vk::UniqueDevice logicalDevice {nullptr};
    static vk::UniqueCommandPool commandPool {nullptr};
    static vk::UniquePipelineCache pipelineCache {nullptr};
    static std::vector<vk::Image> swapchainImages;

    void Context::tearDown() {
        pipelineCache.reset();
        commandPool.reset();
        logicalDevice.reset();
        instance.reset();
        physicalDevice = nullptr;
    }

    void Context::setPhysicalDevice(vk::PhysicalDevice &&_physicalDevice) {
        physicalDevice = _physicalDevice;
    }

    void Context::setLogicalDevice(vk::UniqueDevice &&_logicalDevice) {
        logicalDevice = std::move(_logicalDevice);
    }

    void Context::setCommandPool(vk::UniqueCommandPool &&_commandPool) {
        commandPool = std::move(_commandPool);
    }

    void Context::setPipelineCache(vk::UniquePipelineCache &&_pipelineCache) {
        pipelineCache = std::move(_pipelineCache);
    }

    void Context::setInstance(vk::UniqueInstance &&_instance) {
        instance = std::move(_instance);
    }

    void Context::setSwapchainImages(std::vector<vk::Image> _swapchainImages) {
        swapchainImages = std::move(_swapchainImages);
    }

    auto Context::getPhysicalDevice() -> vk::PhysicalDevice {
        return physicalDevice;
    }

    auto Context::getLogicalDevice() -> vk::Device {
        return logicalDevice.get();
    }

    auto Context::getCommandPool() -> vk::CommandPool {
        return commandPool.get();
    }

    auto Context::getPipelineCache() -> vk::PipelineCache {
        return pipelineCache.get();
    }

    auto Context::getInstance() -> vk::Instance {
        return instance.get();
    }

    auto Context::getSwapchainImages() -> const std::vector<vk::Image> & {
        if (swapchainImages.empty()) {
            throw std::runtime_error("ANUS");
        }
        return swapchainImages;
    }
}

#pragma clang diagnostic pop