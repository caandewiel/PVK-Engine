//
//  KTXLoader.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 17/01/2021.
//

#ifndef KTXLoader_hpp
#define KTXLoader_hpp

#include <stdio.h>
#include <string>
#include <gli.hpp>
#include <vulkan/vulkan.hpp>

#include "../buffer/buffer.hpp"
#include "../texture/texture.hpp"

namespace pvk::ktx {
        pvk::Texture load(const vk::Queue &graphicsQueue,
                                   const std::string &filePath);
    }

#endif /* KTXLoader_hpp */
