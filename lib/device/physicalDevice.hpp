//
//  physicalDevice.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 16/01/2021.
//

#ifndef physicalDevice_hpp
#define physicalDevice_hpp

#include <stdio.h>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <vulkan/vulkan.hpp>

namespace pvk {
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };
    
    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };
    
    namespace device {
        namespace physical {
            QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface);
            vk::PhysicalDevice initialize(const vk::Instance &instance,
                                          const vk::SurfaceKHR &surface,
                                          std::vector<const char*> deviceExtensions);
        }
    }
}

#endif /* physicalDevice_hpp */
