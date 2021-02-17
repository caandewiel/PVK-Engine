//
//  KTXLoader.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 17/01/2021.
//

#include "KTXLoader.hpp"

namespace pvk::ktx {
    auto load(const vk::Queue &graphicsQueue,
              const std::string &filePath) -> std::unique_ptr<pvk::Texture> {
        gli::texture_cube textureCube(gli::load(filePath.c_str()));

        auto texture = std::make_unique<Texture>();

        pvk::buffer::texture::create(graphicsQueue, textureCube, *texture);

        return texture;
    }
}
