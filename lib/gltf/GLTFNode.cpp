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

    auto Node::getGlobalMatrix() const -> const glm::mat4 {
        auto localMatrix = this->getLocalMatrix();
        auto currentParent = this->parent;

        while (currentParent) {
            localMatrix = currentParent->getLocalMatrix() * localMatrix;
            currentParent = currentParent->parent;
        }

        return localMatrix;
    }

    auto Node::getLocalMatrix() const -> const glm::mat4 {
        if (this->matrix == glm::mat4(1.0F)) {
            return glm::translate(glm::mat4(1.0F), this->translation)
                   * glm::mat4(this->rotation)
                   * glm::scale(glm::mat4(1.0F), this->scale)
                   * this->matrix;
        }

        return this->matrix;
    }
}
