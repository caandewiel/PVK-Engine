//
// Created by Christian aan de Wiel on 30/01/2021.
//

#ifndef PVK_GLTFSKIN_HPP
#define PVK_GLTFSKIN_HPP

#include <glm/glm.hpp>
#include "GLTFNode.hpp"

namespace pvk::gltf {
    struct Skin {
        Node* skeletonRoot = nullptr;
        uint32_t skinIndex;
        std::vector<glm::mat4> inverseBindMatrices;
        std::vector<uint32_t> jointsIndices;
    };
}

#endif //PVK_GLTFSKIN_HPP
