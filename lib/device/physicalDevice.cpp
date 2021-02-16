//
//  physicalDevice.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 16/01/2021.
//

#include "physicalDevice.hpp"

namespace pvk::device::physical {
    inline auto checkDeviceExtensionSupport(const vk::PhysicalDevice &device,
                                            const std::vector<const char *> &deviceExtensions) -> bool {
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto &extension : device.enumerateDeviceExtensionProperties()) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    auto findQueueFamilies(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface) -> QueueFamilyIndices {
        QueueFamilyIndices indices;

        auto queueFamilies = device.getQueueFamilyProperties();

        int i = 0;
        for (const auto &queueFamily : queueFamilies) {
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

    inline auto
    querySwapChainSupport(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface) -> SwapChainSupportDetails {
        SwapChainSupportDetails details;
        details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
        details.formats = device.getSurfaceFormatsKHR(surface);
        details.presentModes = device.getSurfacePresentModesKHR(surface);

        return details;
    }

    inline auto isDeviceSuitable(const vk::PhysicalDevice &device,
                                 const vk::SurfaceKHR &surface,
                                 const std::vector<const char *>& deviceExtensions) -> bool {
        QueueFamilyIndices indices = findQueueFamilies(device, surface);

        bool extensionsSupported = checkDeviceExtensionSupport(device, deviceExtensions);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    auto initialize(const vk::Instance &instance,
                                  const vk::SurfaceKHR &surface,
                                  const std::vector<const char *>& deviceExtensions) -> vk::PhysicalDevice {
        vk::PhysicalDevice physicalDevice{};
        auto devices = instance.enumeratePhysicalDevices();

        if (devices.empty()) {
            throw std::runtime_error("Failed to find GPU with Vulkan support.");
        }

        for (const auto &device : devices) {
            if (isDeviceSuitable(device, surface, deviceExtensions)) {
                physicalDevice = device;
                break;
            }
        }

        if (!physicalDevice) {
            throw std::runtime_error("Failed to find suitable GPU.");
        }

        return physicalDevice;
    }
}
