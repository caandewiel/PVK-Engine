//
//  texture.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 17/01/2021.
//

#include "texture.hpp"

namespace pvk {
    Texture::Texture() {}
    
    vk::DescriptorImageInfo* Texture::getDescriptorImageInfo() {
        this->descriptorImageInfo = {this->sampler.get(), this->imageView.get(), vk::ImageLayout::eShaderReadOnlyOptimal};
        
        return &this->descriptorImageInfo;
    }
}
