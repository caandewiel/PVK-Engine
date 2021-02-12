//
//  KTXLoader.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 17/01/2021.
//

#include "KTXLoader.hpp"

namespace pvk::ktx {
    pvk::Texture load(const vk::Queue &graphicsQueue,
                      const std::string &filePath) {
        gli::texture_cube textureCube(gli::load(filePath.c_str()));

        pvk::Texture texture{};

        pvk::buffer::texture::create(graphicsQueue, textureCube, texture);

        return texture;
    }
}
