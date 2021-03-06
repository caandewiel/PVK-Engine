//
//  KTXLoader.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 17/01/2021.
//

#ifndef KTXLoader_hpp
#define KTXLoader_hpp


#include <string>
#include <vulkan/vulkan.hpp>

#include "../buffer/buffer.hpp"
#include "../texture/texture.hpp"

namespace pvk::ktx {
    auto load(const vk::Queue &graphicsQueue,
              const std::string &filePath) -> std::unique_ptr<pvk::Texture>;
}

#endif /* KTXLoader_hpp */
