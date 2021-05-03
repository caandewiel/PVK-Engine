//
// Created by Christian aan de Wiel on 03/05/2021.
//

#ifndef PVK_GLTFLOADERMATERIAL_H
#define PVK_GLTFLOADERMATERIAL_H

#include <tiny_gltf/tiny_gltf.h>
#include "GLTFLoaderNode.hpp"

namespace pvk::gltf::loader::material {
    std::unique_ptr<pvk::gltf::Material> getMaterial(
            const tinygltf::Model &model,
            uint32_t materialIndex
    );
}

#endif //PVK_GLTFLOADERMATERIAL_H
