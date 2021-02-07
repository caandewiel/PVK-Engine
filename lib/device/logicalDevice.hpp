//
//  logicalDevice.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 17/01/2021.
//

#ifndef logicalDevice_hpp
#define logicalDevice_hpp

#include <stdio.h>
#include <vulkan/vulkan.hpp>

#include "physicalDevice.hpp"

namespace pvk {
    namespace device {
        namespace logical {
            vk::UniqueDevice create(const vk::PhysicalDevice &physicalDevice,
                                    const QueueFamilyIndices &indices,
                                    const std::vector<const char*> deviceExtensions,
                                    const std::vector<const char*> validationLayers,
                                    const bool enableValidationLayers);
        }
    }
}

#endif /* logicalDevice_hpp */
