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
        struct MaterialFactor {
            glm::vec4 baseColorFactor{};
            float metallicFactor{};
            float roughnessFactor{};
        } materialFactor;

        Texture &getTextureByBinding(uint32_t binding) {
            switch (binding) {
                case 1:
                    return *this->baseColorTexture;
                    break;
                case 2:
                    return *this->normalTexture;
                    break;
                case 3:
                    return *this->metallicRoughnessTexture;
                    break;
                case 4:
                    return *this->occlusionTexture;
                    break;
                case 5:
                    return *this->emissiveTexture;
                    break;
                default:
                    throw std::runtime_error("Invalid binding index.");
            }
        }
    };
}

#endif //PVK_GLTFMATERIAL_HPP
