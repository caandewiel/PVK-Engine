//
//  image.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 17/01/2021.
//

#ifndef image_hpp
#define image_hpp

#include <cstdio>
#include <vulkan/vulkan.hpp>

#include "../context/context.hpp"
#include "../util/util.hpp"

namespace pvk::image {
        void create(uint32_t width,
                    uint32_t height,
                    uint32_t mipLevels,
                    uint32_t arrayLayers,
                    vk::SampleCountFlagBits numSamples,
                    vk::Format format,
                    vk::ImageTiling tiling,
                    vk::ImageUsageFlags usage,
                    vk::MemoryPropertyFlags properties,
                    vk::ImageCreateFlags imageCreateFlags,
                    vk::UniqueImage& image,
                    vk::UniqueDeviceMemory& imageMemory);
        
        void transitionLayout(const vk::CommandBuffer &commandBuffer,
                              const vk::Queue &graphicsQueue,
                              const vk::Image &image,
                              const vk::Format &format,
                              vk::ImageLayout oldLayout,
                              vk::ImageLayout newLayout,
                              uint32_t mipLevels,
                              uint32_t arrayLayers);
    }

#endif /* image_hpp */
