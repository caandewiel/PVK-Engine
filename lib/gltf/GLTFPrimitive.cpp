//
//  PvkGLTFPrimitive.cpp
//  PVK
//
//  Created by Christian aan de Wiel on 10/01/2021.
//

#include "GLTFPrimitive.hpp"

namespace pvk::gltf
{
Primitive::Primitive() = default;

Primitive::Primitive(uint32_t startVertex, uint32_t startIndex, uint32_t vertexCount, uint32_t indexCount)
    : startIndex(startIndex), startVertex(startVertex), indexCount(indexCount), vertexCount(vertexCount)
{
}

Primitive::~Primitive() = default;

const Primitive::Material &Primitive::getMaterial() const
{
    return material;
}

uint32_t Primitive::getStartIndex() const
{
    return startIndex;
}

uint32_t Primitive::getStartVertex() const
{
    return startVertex;
}

uint32_t Primitive::getIndexCount() const
{
    return indexCount;
}

uint32_t Primitive::getVertexCount() const
{
    return vertexCount;
}

const std::vector<vk::UniqueDescriptorSet> &Primitive::getDescriptorSets() const
{
    return descriptorSets;
}

void Primitive::setDescriptorSets(std::vector<vk::UniqueDescriptorSet> &&_descriptorSets)
{
    this->descriptorSets = std::move(_descriptorSets);
}

} // namespace pvk::gltf
