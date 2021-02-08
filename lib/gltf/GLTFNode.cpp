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

    glm::mat4 Node::getGlobalMatrix() const {
        auto localMatrix = this->getLocalMatrix();
        auto currentParent = this->parent;

        while (currentParent) {
            localMatrix = currentParent->getLocalMatrix() * localMatrix;
            currentParent = currentParent->parent;
        }

        return localMatrix;
    }

    glm::mat4 Node::getLocalMatrix() const {
        if (this->matrix == glm::mat4(1.0f)) {
            return glm::translate(glm::mat4(1.0f), this->translation)
                   * glm::mat4(this->rotation)
                   * glm::scale(glm::mat4(1.0f), this->scale)
                   * this->matrix;
        } else {
            return this->matrix;
        }
    }
}
