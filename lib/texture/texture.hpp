//
//  texture.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 17/01/2021.
//

#ifndef texture_hpp
#define texture_hpp

#include <stdio.h>
#include <gli.hpp>
#include <vulkan/vulkan.hpp>

namespace pvk {
    class Texture {
    public:
        Texture() = default;
        ~Texture() = default;
        auto getDescriptorImageInfo() -> vk::DescriptorImageInfo*;
        
        vk::UniqueImage image {};
        vk::UniqueDeviceMemory imageMemory {};
        vk::UniqueSampler sampler {};
        vk::UniqueImageView imageView {};
        
    private:
        vk::DescriptorImageInfo descriptorImageInfo{};
    };
}

#endif /* texture_hpp */
