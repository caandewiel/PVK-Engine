//
// Created by Christian aan de Wiel on 03/05/2021.
//

#ifndef PVK_GLTFLOADERNODE_HPP
#define PVK_GLTFLOADERNODE_HPP

#include <vector>
#include <vulkan/vulkan.hpp>

#include "../GLTFNode.hpp"
#include "../GLTFObject.hpp"

inline glm::vec3 getTranslation(const tinygltf::Node &node) {
    constexpr uint8_t numberOfElementInTranslationVector = 3;

    if (node.translation.size() == numberOfElementInTranslationVector) {
        return glm::make_vec3(node.translation.data());
    }

    return glm::vec3(0.0F);
}

inline glm::mat4 getRotation(const tinygltf::Node &node) {
    constexpr uint8_t numberOfElementInRotationVector = 4;

    if (node.rotation.size() == numberOfElementInRotationVector) {
        glm::quat quaternion = glm::make_quat(node.rotation.data());

        return glm::mat4(quaternion);
    }

    return glm::mat4(1.0F);
}

inline glm::vec3 getScale(const tinygltf::Node &node) {
    constexpr uint8_t numberOfElementInScaleVector = 3;

    if (node.scale.size() == numberOfElementInScaleVector) {
        return glm::make_vec3(node.scale.data());
    }

    return glm::vec3(1.0F);
}

inline glm::mat4 getOrientationMatrix(const tinygltf::Node &node) {
    constexpr uint8_t numberOfElementInOrientationMatrix = 16;

    if (node.matrix.size() == numberOfElementInOrientationMatrix) {
        return glm::make_mat4x4(node.matrix.data());
    }

    return glm::mat4(1.0F);
}

inline std::shared_ptr<pvk::gltf::Skin>
getSkin(const tinygltf::Model &model, const tinygltf::Node &node) {
    if (node.skin == -1) {
        return nullptr;
    }

    const auto &skin = model.skins[node.skin];
    auto result = std::make_shared<pvk::gltf::Skin>();
    result->skinIndex = node.skin;
    result->jointsIndices.reserve(skin.joints.size());

    for (const auto jointIndex : skin.joints) {
        result->jointsIndices.emplace_back(jointIndex);
    }

    if (skin.inverseBindMatrices > -1) {
        const auto &accessor = model.accessors[skin.inverseBindMatrices];
        const auto &bufferView = model.bufferViews[accessor.bufferView];
        const auto &buffer = model.buffers[bufferView.buffer];

        result->inverseBindMatrices.resize(accessor.count);
        memcpy(
                result->inverseBindMatrices.data(),
                &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                accessor.count * sizeof(glm::mat4)
        );
    }

    return result;
}

inline std::shared_ptr<pvk::gltf::Node>
initializeNode(uint32_t nodeIndex, const std::shared_ptr<pvk::gltf::Node> &parent, const tinygltf::Node &node) {
    auto resultNode = std::make_shared<pvk::gltf::Node>();
    resultNode->parent = parent;
    resultNode->skinIndex = node.skin;
    resultNode->nodeIndex = nodeIndex;
    resultNode->name = node.name;

    return resultNode;
}

/**
 * Loads one specific node from the tinygltf model.
 * @param model
 * @param primitiveLookup
 * @param nodeIndex
 * @param graphicsQueue
 * @param object
 * @param parent
 * @return
 */
inline std::shared_ptr<pvk::gltf::Node> loadNode(
        const std::shared_ptr<tinygltf::Model> &model,
        const std::vector<std::vector<std::shared_ptr<pvk::gltf::Primitive>>> &primitiveLookup,
        uint32_t nodeIndex,
        vk::Queue &graphicsQueue,
        pvk::gltf::Object &object,
        const std::shared_ptr<pvk::gltf::Node> &parent = nullptr
) {
    const auto &node = model->nodes[nodeIndex];
    auto resultNode = initializeNode(nodeIndex, parent, node);

    // First load all children recursively
    resultNode->children.reserve(node.children.size());

    for (const auto &child : node.children) {
        resultNode->children.emplace_back(
                loadNode(model, primitiveLookup, child, graphicsQueue, object, resultNode)
        );
    }

    resultNode->translation = getTranslation(node);
    resultNode->rotation = getRotation(node);
    resultNode->scale = getScale(node);
    resultNode->matrix = getOrientationMatrix(node);

    // Filter out all lights and cameras
    if (node.mesh == -1) {
        return resultNode;
    }

    auto &mesh = model->meshes[node.mesh];

    resultNode->mesh = std::make_unique<pvk::Mesh>();
    resultNode->primitives.reserve(mesh.primitives.size());

    for (const auto &primitive : primitiveLookup[node.mesh]) {
        resultNode->primitives.emplace_back(primitive);
    }

    object.skinLookup[resultNode->skinIndex] = getSkin(*model, node);

    return resultNode;
}

namespace pvk::gltf::loader::node {
    /**
     * Loads all nodes from the GLTF file.
     * @param model
     * @param primitiveLookup
     * @param graphicsQueue
     * @param object
     * @return Vector containing all nodes from the GLTF file.
     */
    inline std::vector<std::shared_ptr<pvk::gltf::Node>> loadNodes(
            const std::shared_ptr<tinygltf::Model> &model,
            const std::vector<std::vector<std::shared_ptr<pvk::gltf::Primitive>>> &primitiveLookup,
            vk::Queue &graphicsQueue,
            pvk::gltf::Object &object
    ) {
        std::vector<std::shared_ptr<pvk::gltf::Node>> nodes{};
        const auto &scene = model->scenes[model->defaultScene];

        for (const auto &nodeIndex : scene.nodes) {
            auto node = loadNode(model, primitiveLookup, nodeIndex, graphicsQueue, object);

            if (node->children.empty() && node->mesh == nullptr) {
                // Don't add this, most likely light or camera.
            } else {
                nodes.emplace_back(node);
            }
        }

        return nodes;
    }
}  // namespace pvk::gltf::loader::node

#endif //PVK_GLTFLOADERNODE_HPP
