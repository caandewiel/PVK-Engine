//
//  util.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 16/01/2021.
//

#include "util.hpp"

namespace pvk {
    namespace util {
        std::vector<char> readFile(const std::string& filename) {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                throw std::runtime_error("failed to open file!");
            }

            size_t fileSize = (size_t)file.tellg();
            std::vector<char> buffer(fileSize);

            file.seekg(0);
            file.read(buffer.data(), fileSize);

            file.close();

            return buffer;
        }
        
        std::vector<vk::CommandBuffer> beginOneTimeCommandBuffer() {
            // This will only be one commandbuffer
            std::vector<vk::CommandBuffer> commandBuffers = Context::getLogicalDevice().allocateCommandBuffers({Context::getCommandPool(), vk::CommandBufferLevel::ePrimary, 1});

            commandBuffers.front().begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr});

            return commandBuffers;
        }
        
        void endSingleTimeCommands(const std::vector<vk::CommandBuffer> &commandBuffers,
                                   const vk::Queue &graphicsQueue)
        {
            commandBuffers.front().end();
            graphicsQueue.submit(vk::SubmitInfo{ 0, nullptr, nullptr, 1, &commandBuffers.front() }, vk::Fence());
            graphicsQueue.waitIdle();
        }
        
        vk::Format findSupportedFormat(const vk::PhysicalDevice &physicalDevice,
                                       const std::vector<vk::Format> &candidates,
                                       vk::ImageTiling tiling,
                                       vk::FormatFeatureFlags features)
        {
            for (vk::Format format : candidates) {
                vk::FormatProperties properties = physicalDevice.getFormatProperties(format);

                if (tiling == vk::ImageTiling::eLinear && (properties.linearTilingFeatures & features) == features) {
                    return format;
                } else if (tiling == vk::ImageTiling::eOptimal && (properties.optimalTilingFeatures & features) == features) {
                    return format;
                }
            }

            throw std::runtime_error("Failed to find supported format");
        }
    }
}
