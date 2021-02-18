//
//  context.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 20/01/2021.
//

#ifndef context_hpp
#define context_hpp

#include <cstdio>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace pvk {
    class Context {
    public:
        static void tearDown();

        static void setPhysicalDevice(vk::PhysicalDevice &&_physicalDevice);

        static void setLogicalDevice(vk::UniqueDevice &&_logicalDevice);

        static void setCommandPool(vk::UniqueCommandPool &&_commandPool);

        static void setPipelineCache(vk::UniquePipelineCache &&_pipelineCache);

        static void setInstance(vk::UniqueInstance &&_instance);

        static void setSwapchainImages(std::vector<vk::Image> _swapchainImages);
        
        static auto getPhysicalDevice() -> vk::PhysicalDevice;

        static auto getLogicalDevice() -> vk::Device;

        static auto getCommandPool() -> vk::CommandPool;

        static auto getPipelineCache() -> vk::PipelineCache;

        static auto getInstance() -> vk::Instance;

        static const std::vector<vk::Image> &getSwapchainImages();

    private:
        Context() = default;
    };
}

#endif /* context_hpp */
