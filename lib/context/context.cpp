//
//  context.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 20/01/2021.
//

#include "context.hpp"

namespace pvk {
    Context* Context::context = 0;
    const vk::PhysicalDevice* Context::physicalDevice = 0;
    const vk::UniqueDevice* Context::logicalDevice = 0;
    const vk::UniqueCommandPool* Context::commandPool = 0;
    const vk::PipelineCache* Context::pipelineCache = 0;
    const std::vector<vk::Image>* Context::swapchainImages = 0;
    
    Context* Context::get() {
        if (context == 0) {
            context = new Context();
        }
        
        return context;
    }
    
    Context::Context() {};
    
    void Context::setPhysicalDevice(const vk::PhysicalDevice *physicalDevice) {
        Context::physicalDevice = physicalDevice;
    }
    
    void Context::setLogicalDevice(const vk::UniqueDevice *logicalDevice) {
        Context::logicalDevice = logicalDevice;
    }
    
    void Context::setCommandPool(const vk::UniqueCommandPool *commandPool) {
        Context::commandPool = commandPool;
    }
    
    void Context::setPipelineCache(const vk::PipelineCache *pipelineCache) {
        Context::pipelineCache = pipelineCache;
    }
    
    void Context::setSwapchainImages(const std::vector<vk::Image>* swapchainImages) {
        Context::swapchainImages = swapchainImages;
    }
    
    const vk::PhysicalDevice Context::getPhysicalDevice() {
        if (physicalDevice == 0) {
            throw std::runtime_error("Physical device is undefined.");
        }
        
        return *physicalDevice;
    }
    
    const vk::Device Context::getLogicalDevice() {
        if (logicalDevice == 0) {
            throw std::runtime_error("Logical device is undefined.");
        }
        
        return logicalDevice->get();
    }
    
    const vk::CommandPool Context::getCommandPool() {
        if (commandPool == 0) {
            throw std::runtime_error("Command pool is undefined");
        }
        
        return commandPool->get();
    }
    
    const vk::PipelineCache Context::getPipelineCache() {
        if (pipelineCache == 0) {
            throw std::runtime_error("Pipeline cache is undefined");
        }
        
        return *pipelineCache;
    }
    
    const std::vector<vk::Image> Context::getSwapchainImages() {
        if (swapchainImages == 0) {
            throw std::runtime_error("Swapchain images are undefined");
        }
        
        return *swapchainImages;
    }
}
