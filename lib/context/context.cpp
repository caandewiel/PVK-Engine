//
//  context.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 20/01/2021.
//

#include "context.hpp"

namespace pvk {
    const std::vector<vk::Image>* Context::swapchainImages = 0;
    vk::PhysicalDevice Context::physicalDevice;
    vk::UniqueDevice Context::logicalDevice;
    vk::UniqueCommandPool Context::commandPool;
    vk::UniquePipelineCache Context::pipelineCache;

    void Context::setSwapchainImages(const std::vector<vk::Image>* swapchainImages) {
        Context::swapchainImages = swapchainImages;
    }
    
    const std::vector<vk::Image> Context::getSwapchainImages() {
        if (swapchainImages == 0) {
            throw std::runtime_error("Swapchain images are undefined");
        }
        
        return *swapchainImages;
    }
}
