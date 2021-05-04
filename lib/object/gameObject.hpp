//
// Created by Christian aan de Wiel on 04/05/2021.
//

#ifndef PVK_GAMEOBJECT_H
#define PVK_GAMEOBJECT_H

#include <vector>
#include <vulkan/vulkan.hpp>

#include "../mesh/vertex.hpp"

namespace pvk::object {
    class Mesh {
    public:
        Mesh(std::vector<Vertex> &vertices, std::vector<uint32_t> &indices);

        Mesh(const Mesh &other) = delete;

        Mesh(Mesh &&other) = default;

        Mesh &operator=(const Mesh &other) = delete;

        Mesh &operator=(Mesh &&other) = default;

        ~Mesh() = default;

        [[nodiscard]] const std::vector<Vertex> &getVertices() const;

        [[nodiscard]] const std::vector<uint32_t> &getIndices() const;

        [[nodiscard]] const vk::Buffer &getVertexBuffer() const;

        [[nodiscard]] const vk::DeviceMemory &getVertexBufferMemory() const;

        [[nodiscard]] const vk::Buffer &getIndexBuffer() const;

        [[nodiscard]] const vk::DeviceMemory &getIndexBufferMemory() const;

    private:
        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;
        vk::UniqueBuffer m_vertexBuffer;
        vk::UniqueDeviceMemory m_vertexBufferMemory;
        vk::UniqueBuffer m_indexBuffer;
        vk::UniqueDeviceMemory m_indexBufferMemory;
    };

    class Transform {
    public:
        Transform() = default;

        Transform(const Transform &other) = delete;

        Transform(Transform &&other) = default;

        Transform &operator=(const Transform &other) = delete;

        Transform &operator=(Transform &&other) = default;

        ~Transform() = default;

        [[nodiscard]] const glm::vec3 &getTranslation() const;

        [[nodiscard]] const glm::vec3 &getScale() const;

        [[nodiscard]] const glm::mat4 &getRotation() const;

    private:
        glm::vec3 m_translation = glm::vec3(0.0F);
        glm::vec3 m_scale = glm::vec3(1.0F);
        glm::mat4 m_rotation = glm::mat4(1.0F);
    };

    class GameObject {
    public:
        GameObject(std::unique_ptr<Mesh> mesh, std::unique_ptr<Transform> transform);

        GameObject(const GameObject &other) = delete;

        GameObject(GameObject &&other) = default;

        GameObject &operator=(const GameObject &other) = delete;

        GameObject &operator=(GameObject &&other) = default;

        ~GameObject() = default;

        [[nodiscard]] const Mesh &getMesh() const;

        [[nodiscard]] const Transform &getTransform() const;

    private:
        std::unique_ptr<Mesh> m_mesh;
        std::unique_ptr<Transform> m_transform;
    };

    class Cube : private GameObject {
    };
}  // namespace pvk::object

#endif //PVK_GAMEOBJECT_H
