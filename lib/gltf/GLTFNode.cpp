//
//  PvkGLTFNode.cpp
//  PVK
//
//  Created by Christian aan de Wiel on 10/01/2021.
//

#include "GLTFNode.hpp"

namespace pvk::gltf
{
glm::mat4 Node::getGlobalMatrix() const
{
    auto localMatrix = this->getLocalMatrix();
    auto currentParent = this->parent;

    while (currentParent.lock())
    {
        localMatrix = currentParent.lock()->getLocalMatrix() * localMatrix;
        currentParent = currentParent.lock()->parent;
    }

    return localMatrix;
}

glm::mat4 Node::getLocalMatrix() const
{
    if (this->matrix == glm::mat4(1.0F))
    {
        return glm::translate(glm::mat4(1.0F), this->translation) * glm::mat4(this->rotation) *
               glm::scale(glm::mat4(1.0F), this->scale) * this->matrix;
    }

    return this->matrix;
}

} // namespace pvk::gltf
