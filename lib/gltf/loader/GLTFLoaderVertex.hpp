//
// Created by Christian aan de Wiel on 03/05/2021.
//

#ifndef PVK_GLTFLOADERVERTEX_HPP
#define PVK_GLTFLOADERVERTEX_HPP

#define FIELD_VERTEX_POSITION "POSITION"
#define FIELD_VERTEX_NORMAL "NORMAL"
#define FIELD_VERTEX_COLOR_0 "COLOR_0"
#define FIELD_VERTEX_TEXCOORD_0 "TEXCOORD_0"
#define FIELD_VERTEX_TEXCOORD_1 "TEXCOORD_1"
#define FIELD_VERTEX_JOINTS_0 "JOINTS_0"
#define FIELD_VERTEX_WEIGHTS_0 "WEIGHTS_0"

#include <tiny_gltf/tiny_gltf.h>
#include <glm/glm.hpp>
#include "GLTFLoaderNode.hpp"

namespace pvk::gltf::loader::vertex {
    Vertex
    generateVertexByPrimitive(
            const std::shared_ptr<tinygltf::Model> &model,
            const tinygltf::Primitive &primitive,
            uint32_t index
    );
}  // namespace pvk::gltf::loader::vertex

#endif //PVK_GLTFLOADERVERTEX_HPP
