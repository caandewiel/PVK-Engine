//
//  PvkGLTFNode.cpp
//  PVK
//
//  Created by Christian aan de Wiel on 10/01/2021.
//

#include "GLTFNode.hpp"

namespace pvk::gltf {
    Node::Node() = default;

    Node::~Node() = default;

    glm::mat4 Node::getLocalMatrix() const {
        auto localMatrix = this->matrix;
        auto currentParent = this->parent;

        while (currentParent) {
            localMatrix = currentParent->matrix * localMatrix;
            currentParent = currentParent->parent;
        }

        return localMatrix;
    }
}
