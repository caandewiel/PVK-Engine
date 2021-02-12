//
// Created by Christian aan de Wiel on 10/02/2021.
//

#ifndef PVK_GLTFMATERIAL_HPP
#define PVK_GLTFMATERIAL_HPP

#include "../texture/texture.hpp"

namespace pvk::gltf {
    struct Material {
        Texture* baseColorTexture{};
        Texture* metallicRoughnessTexture{};
        Texture* occlusionTexture{};
        glm::vec4 baseColorFactor{};
        float metallicFactor{};
        float roughnessFactor{};
    };
}

#endif //PVK_GLTFMATERIAL_HPP
