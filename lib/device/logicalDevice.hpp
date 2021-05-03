//
//  logicalDevice.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 17/01/2021.
//

#ifndef logicalDevice_hpp
#define logicalDevice_hpp


#include <vulkan/vulkan.hpp>

#include "physicalDevice.hpp"

namespace pvk::device::logical {
    auto create(const vk::PhysicalDevice &physicalDevice,
                const QueueFamilyIndices &indices,
                const std::vector<const char *> &deviceExtensions,
                const std::vector<const char *> &validationLayers,
                bool enableValidationLayers) -> vk::UniqueDevice;
}

#endif /* logicalDevice_hpp */
