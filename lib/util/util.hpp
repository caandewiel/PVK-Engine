//
//  util.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 16/01/2021.
//

#ifndef util_hpp
#define util_hpp

#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "../context/context.hpp"

namespace pvk::util
{
class NoCopy
{
public:
    NoCopy() = default;

    NoCopy(const NoCopy &other) = delete;
    NoCopy(NoCopy &&other) = default;

    NoCopy &operator=(const NoCopy &other) = delete;
    NoCopy &operator=(NoCopy &&other) = default;

    ~NoCopy() = default;
};

std::vector<char> readFile(const std::string &filename);

std::vector<vk::CommandBuffer> beginOneTimeCommandBuffer();

void endSingleTimeCommands(const std::vector<vk::CommandBuffer> &commandBuffers, const vk::Queue &graphicsQueue);

vk::Format findSupportedFormat(const vk::PhysicalDevice &physicalDevice,
                               const std::vector<vk::Format> &candidates,
                               vk::ImageTiling tiling,
                               vk::FormatFeatureFlags features);
} // namespace pvk::util

#endif /* util_hpp */
