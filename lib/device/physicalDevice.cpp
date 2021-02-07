//
//  physicalDevice.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 16/01/2021.
//

#include "physicalDevice.hpp"

namespace pvk {
    namespace device {
        namespace physical {
            inline bool checkDeviceExtensionSupport(const vk::PhysicalDevice& device, const std::vector<const char *> deviceExtensions) {
                std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

                for (const auto& extension : device.enumerateDeviceExtensionProperties()) {
                    requiredExtensions.erase(extension.extensionName);
                }

                return requiredExtensions.empty();
            }
            
            QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface) {
                QueueFamilyIndices indices;

                auto queueFamilies = device.getQueueFamilyProperties();

                int i = 0;
                for (const auto& queueFamily : queueFamilies) {
                    if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
                        indices.graphicsFamily = i;
                    }

                    if (queueFamily.queueCount > 0 && device.getSurfaceSupportKHR(i, surface)) {
                        indices.presentFamily = i;
                    }

                    if (indices.isComplete()) {
                        break;
                    }

                    i++;
                }

                return indices;
            }
            
            inline SwapChainSupportDetails querySwapChainSupport(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface) {
                SwapChainSupportDetails details;
                details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
                details.formats = device.getSurfaceFormatsKHR(surface);
                details.presentModes = device.getSurfacePresentModesKHR(surface);

                return details;
            }
            
            inline bool isDeviceSuitable(const vk::PhysicalDevice& device,
                                         const vk::SurfaceKHR &surface,
                                         const std::vector<const char *> deviceExtensions)
            {
                QueueFamilyIndices indices = findQueueFamilies(device, surface);

                bool extensionsSupported = checkDeviceExtensionSupport(device, deviceExtensions);

                bool swapChainAdequate = false;
                if (extensionsSupported) {
                    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
                    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
                }

                return indices.isComplete() && extensionsSupported && swapChainAdequate;
            }
            
            vk::PhysicalDevice initialize(const vk::Instance &instance,
                                          const vk::SurfaceKHR &surface,
                                          const std::vector<const char *> deviceExtensions)
            {
                vk::PhysicalDevice physicalDevice {};
                auto devices = instance.enumeratePhysicalDevices();
                
                if (devices.size() == 0) {
                    throw std::runtime_error("Failed to find GPU with Vulkan support.");
                }

                for (const auto& device : devices) {
                    if (isDeviceSuitable(device, surface, deviceExtensions)) {
                        physicalDevice = device;
                        break;
                    }
                }

                if (!physicalDevice) {
                    throw std::runtime_error("Failed to find suitable GPU.");
                } else {
                    return physicalDevice;
                }
            }
        }
    }
}
