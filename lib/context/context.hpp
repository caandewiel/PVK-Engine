//
//  context.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 20/01/2021.
//

#ifndef context_hpp
#define context_hpp

#include <stdio.h>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace pvk {
    class Context {
    private:
        static Context* context;
        static const vk::PhysicalDevice *physicalDevice;
        static const vk::UniqueDevice *logicalDevice;
        static const vk::UniqueCommandPool *commandPool;
        static const vk::PipelineCache *pipelineCache;
        static const std::vector<vk::Image> *swapchainImages;
        
        Context();
    public:
        static Context* get();
        
        static void setPhysicalDevice(const vk::PhysicalDevice *physicalDevice);
        static void setLogicalDevice(const vk::UniqueDevice *logicalDevice);
        static void setCommandPool(const vk::UniqueCommandPool *commandPool);
        static void setPipelineCache(const vk::PipelineCache *pipelineCache);
        static void setSwapchainImages(const std::vector<vk::Image>* swapchainImages);
        
        static const vk::PhysicalDevice getPhysicalDevice();
        static const vk::Device getLogicalDevice();
        static const vk::CommandPool getCommandPool();
        static const vk::PipelineCache getPipelineCache();
        static const std::vector<vk::Image> getSwapchainImages();
    };
}

#endif /* context_hpp */
