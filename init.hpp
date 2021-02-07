//
//  init.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 14/01/2021.
//

#ifndef init_h
#define init_h

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <string>

namespace vk {
    namespace init {
        GLFWwindow* window(uint32_t width, uint32_t height, std::string name) {
            GLFWwindow* window;
            glfwInit();

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

            window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
            
            return window;
        }
        
        vk::UniqueSurfaceKHR surface(vk::UniqueInstance &instance, GLFWwindow* window) {
            VkSurfaceKHR _surface;
            
            glfwCreateWindowSurface(instance.get(), window, nullptr, &_surface);
            
            vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> _deleter(instance.get());
            auto surface = vk::UniqueSurfaceKHR(vk::SurfaceKHR(_surface ), _deleter);
            
            return surface;
        }
    }
}

#endif /* init_h */
