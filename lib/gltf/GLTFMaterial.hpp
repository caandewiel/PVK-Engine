//
// Created by Christian aan de Wiel on 10/02/2021.
//

#ifndef PVK_GLTFMATERIAL_HPP
#define PVK_GLTFMATERIAL_HPP

#include "../texture/texture.hpp"

namespace pvk::gltf {
    struct Material {
        std::shared_ptr<Texture> baseColorTexture{};
        std::shared_ptr<Texture> metallicRoughnessTexture{};
        std::shared_ptr<Texture> occlusionTexture{};
        std::shared_ptr<Texture> normalTexture{};
        std::shared_ptr<Texture> emissiveTexture{};
        glm::vec4 baseColorFactor{};
        float metallicFactor{};
        float roughnessFactor{};
    };
}

#endif //PVK_GLTFMATERIAL_HPP
