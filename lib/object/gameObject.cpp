//
// Created by Christian aan de Wiel on 04/05/2021.
//

#include "gameObject.hpp"
#include "../buffer/buffer.hpp"

namespace pvk::object {
    Mesh::Mesh(
            std::vector<Vertex> &vertices,
            std::vector<uint32_t> &indices
    ) : m_vertices(std::move(vertices)), m_indices(std::move(indices)) {
        auto vertexBuffer = buffer::vertex::create(this->m_vertices);
        this->m_vertexBuffer = std::move(vertexBuffer.first);
        this->m_vertexBufferMemory = std::move(vertexBuffer.second);

        auto indexBuffer = buffer::index::create(this->m_indices);
        this->m_indexBuffer = std::move(indexBuffer.first);
        this->m_indexBufferMemory = std::move(indexBuffer.second);
    }

    const std::vector<Vertex> &Mesh::getVertices() const {
        return this->m_vertices;
    }

    const std::vector<uint32_t> &Mesh::getIndices() const {
        return this->m_indices;
    }

    const vk::Buffer &Mesh::getVertexBuffer() const {
        return this->m_vertexBuffer.get();
    }

    const vk::DeviceMemory &Mesh::getVertexBufferMemory() const {
        return this->m_vertexBufferMemory.get();
    }

    const vk::Buffer &Mesh::getIndexBuffer() const {
        return this->m_indexBuffer.get();
    }

    const vk::DeviceMemory &Mesh::getIndexBufferMemory() const {
        return this->m_indexBufferMemory.get();
    }

    GameObject::GameObject(
            std::unique_ptr<Mesh> mesh,
            std::unique_ptr<Transform> transform
    ) : m_mesh(std::move(mesh)), m_transform(std::move(transform)) {

    }

    const Mesh &GameObject::getMesh() const {
        return *this->m_mesh;
    }

    const Transform &GameObject::getTransform() const {
        return *this->m_transform;
    }

    const glm::vec3 &Transform::getTranslation() const {
        return this->m_translation;
    }

    const glm::vec3 &Transform::getScale() const {
        return this->m_scale;
    }

    const glm::mat4 &Transform::getRotation() const {
        return this->m_rotation;
    }
}  // namespace pvk::object
