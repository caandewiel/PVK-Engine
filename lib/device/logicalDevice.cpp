//
//  logicalDevice.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 17/01/2021.
//

#include "logicalDevice.hpp"

namespace pvk {
    namespace device {
        namespace logical {
            vk::UniqueDevice create(const vk::PhysicalDevice &physicalDevice,
                                    const QueueFamilyIndices &indices,
                                    const std::vector<const char*> deviceExtensions,
                                    const std::vector<const char*> validationLayers,
                                    const bool enableValidationLayers)
            {
                std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
                std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

                float queuePriority = 1.0f;

                for (uint32_t queueFamily : uniqueQueueFamilies) {
                    queueCreateInfos.push_back({
                        vk::DeviceQueueCreateFlags(),
                        queueFamily,
                        1, // queueCount
                        &queuePriority
                    });
                }

                auto deviceFeatures = vk::PhysicalDeviceFeatures();
                auto createInfo = vk::DeviceCreateInfo(
                    vk::DeviceCreateFlags(),
                    static_cast<uint32_t>(queueCreateInfos.size()),
                    queueCreateInfos.data()
                );
                createInfo.pEnabledFeatures = &deviceFeatures;
                createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
                createInfo.ppEnabledExtensionNames = deviceExtensions.data();

                if (enableValidationLayers) {
                    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                    createInfo.ppEnabledLayerNames = validationLayers.data();
                }

                try {
                    return physicalDevice.createDeviceUnique(createInfo);
                }
                catch (vk::SystemError err) {
                    throw std::runtime_error("failed to create logical device!");
                }
            }
        }
    }
}
