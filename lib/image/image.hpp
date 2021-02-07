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
        void create(const uint32_t width,
                    const uint32_t height,
                    const uint32_t mipLevels,
                    const uint32_t arrayLayers,
                    const vk::SampleCountFlagBits numSamples,
                    const vk::Format format,
                    const vk::ImageTiling tiling,
                    const vk::ImageUsageFlags usage,
                    const vk::MemoryPropertyFlags properties,
                    const vk::ImageCreateFlags imageCreateFlags,
                    vk::UniqueImage& image,
                    vk::UniqueDeviceMemory& imageMemory);
        
        void transitionLayout(const vk::CommandBuffer &commandBuffer,
                              const vk::Queue &graphicsQueue,
                              const vk::Image &image,
                              const vk::Format &format,
                              const vk::ImageLayout oldLayout,
                              const vk::ImageLayout newLayout,
                              uint32_t mipLevels,
                              uint32_t arrayLayers);
    }

#endif /* image_hpp */
