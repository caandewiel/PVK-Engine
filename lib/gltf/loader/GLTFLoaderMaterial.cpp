//
// Created by Christian aan de Wiel on 03/05/2021.
//

#include "GLTFLoaderMaterial.hpp"

namespace {
    std::unique_ptr<pvk::Texture> loadTexture(
            const tinygltf::Model &model,
            const int8_t textureIndex
    ) {
        auto texture = std::make_unique<pvk::Texture>();

        if (textureIndex > -1) {
            const auto &baseColorTexture = model.textures[textureIndex];
            const auto &baseColorTextureImage = model.images[baseColorTexture.source];

            pvk::buffer::texture::create(pvk::Context::getGraphicsQueue(), baseColorTextureImage, *texture);
        } else {
            pvk::buffer::texture::createEmpty(pvk::Context::getGraphicsQueue(), *texture);
        }

        return texture;
    }
}  // namespace

namespace pvk::gltf::loader::material {
    std::unique_ptr<pvk::gltf::Material> getMaterial(
            const tinygltf::Model &model,
            uint32_t materialIndex
    ) {
        const auto &material = model.materials[materialIndex];
        auto _material = std::make_unique<gltf::Material>();

        _material->baseColorTexture = loadTexture(model, material.pbrMetallicRoughness.baseColorTexture.index);
        _material->metallicRoughnessTexture = loadTexture(model, material.pbrMetallicRoughness.metallicRoughnessTexture.index);
        _material->occlusionTexture = loadTexture(model, material.occlusionTexture.index);
        _material->normalTexture = loadTexture(model, material.normalTexture.index);
        _material->emissiveTexture = loadTexture(model, material.emissiveTexture.index);
        _material->materialFactor = {glm::make_vec4(material.pbrMetallicRoughness.baseColorFactor.data()),
                                     static_cast<float>(material.pbrMetallicRoughness.metallicFactor),
                                     static_cast<float>(material.pbrMetallicRoughness.roughnessFactor)};

        return _material;
    }
}  // namespace pvk::gltf::loader::material
