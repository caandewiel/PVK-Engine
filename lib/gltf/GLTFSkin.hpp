//
// Created by Christian aan de Wiel on 30/01/2021.
//

#ifndef PVK_GLTFSKIN_HPP
#define PVK_GLTFSKIN_HPP

#include <glm/glm.hpp>
#include "GLTFNode.hpp"

namespace pvk::gltf {
    struct Skin {
        std::shared_ptr<Node> skeletonRoot;
        uint32_t skinIndex = 0;
        std::vector<glm::mat4> inverseBindMatrices;
        std::vector<uint32_t> jointsIndices;
    };
}

#endif //PVK_GLTFSKIN_HPP
