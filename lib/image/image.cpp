//
//  image.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 17/01/2021.
//

#include "image.hpp"

namespace pvk {
    namespace image {
        inline uint32_t findMemoryType(const uint32_t typeFilter,
                                       const vk::MemoryPropertyFlags properties) {
            vk::PhysicalDeviceMemoryProperties memProperties = Context::getPhysicalDevice().getMemoryProperties();

            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
                if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                    return i;
                }
            }

            throw std::runtime_error("Could not find suitable memory type.");
        }
        
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
                    vk::UniqueImage &image,
                    vk::UniqueDeviceMemory &imageMemory)
        {
            image = Context::getLogicalDevice().createImageUnique({
                imageCreateFlags,
                vk::ImageType::e2D,
                format,
                {width, height, 1},
                mipLevels,
                arrayLayers,
                numSamples,
                tiling,
                usage,
                vk::SharingMode::eExclusive,
                {},
            });
            
            vk::MemoryRequirements memoryRequirements = Context::getLogicalDevice().getImageMemoryRequirements(image.get());
            vk::MemoryAllocateInfo memoryAllocateInfo {};
            memoryAllocateInfo.allocationSize = memoryRequirements.size;
            memoryAllocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);
            
            imageMemory = Context::getLogicalDevice().allocateMemoryUnique(memoryAllocateInfo);
            
            Context::getLogicalDevice().bindImageMemory(image.get(), imageMemory.get(), 0);
        }
        
        void transitionLayout(const vk::CommandBuffer &commandBuffer,
                              const vk::Queue &graphicsQueue,
                              const vk::Image &image,
                              const vk::Format &format,
                              const vk::ImageLayout oldLayout,
                              const vk::ImageLayout newLayout,
                              uint32_t mipLevels,
                              uint32_t arrayLayers)
        {
            vk::ImageMemoryBarrier barrier;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.image = image;
            barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, arrayLayers};
            
            vk::PipelineStageFlags sourceStage;
            vk::PipelineStageFlags destinationStage;

            if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
                barrier.srcAccessMask = vk::AccessFlags();
                barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

                sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
                destinationStage = vk::PipelineStageFlagBits::eTransfer;
            } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
                barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

                sourceStage = vk::PipelineStageFlagBits::eTransfer;
                destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
            } else {
                throw std::invalid_argument("Unsupported layout transition.");
            }

            commandBuffer.pipelineBarrier(sourceStage, destinationStage, vk::DependencyFlags(), nullptr, nullptr, barrier);
        }
    }
}
