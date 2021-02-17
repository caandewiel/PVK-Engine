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
        ~Context() = default;

        static auto get() {
            return Context::context;
        }

        static void tearDown() {
//            Context::commandPool.reset();
//            Context::pipelineCache.reset();
//            Context::logicalDevice.reset();
        }

        static void setPhysicalDevice(vk::PhysicalDevice &&_physicalDevice) {
            Context::physicalDevice = _physicalDevice;
        }

        static void setLogicalDevice(vk::UniqueDevice &&_logicalDevice) {
            Context::logicalDevice = std::move(_logicalDevice);
        }

        static void setCommandPool(vk::UniqueCommandPool &&_commandPool) {
            Context::commandPool = std::move(_commandPool);
        }

        static void setPipelineCache(vk::UniquePipelineCache &&_pipelineCache) {
            Context::pipelineCache = std::move(_pipelineCache);
        }

        static void setSwapchainImages(const std::vector<vk::Image>* swapchainImages);
        
        static auto getPhysicalDevice() -> vk::PhysicalDevice {
            return Context::physicalDevice;
        }

        static auto getLogicalDevice() -> vk::Device {
            return Context::logicalDevice.get();
        }

        static auto getCommandPool() -> vk::CommandPool {
            return Context::commandPool.get();
        }

        static auto getPipelineCache() -> vk::PipelineCache {
            return Context::pipelineCache.get();
        }

        static const std::vector<vk::Image> getSwapchainImages();

    private:
        static const Context context;
        static vk::PhysicalDevice physicalDevice;
        static vk::UniqueDevice logicalDevice;
        static vk::UniqueCommandPool commandPool;
        static vk::UniquePipelineCache pipelineCache;
        static const std::vector<vk::Image> *swapchainImages;

        Context() = default;
    };
}

#endif /* context_hpp */
