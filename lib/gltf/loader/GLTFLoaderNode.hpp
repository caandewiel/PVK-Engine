//
// Created by Christian aan de Wiel on 03/05/2021.
//

#ifndef PVK_GLTFLOADERNODE_HPP
#define PVK_GLTFLOADERNODE_HPP

#include <vector>
#include <vulkan/vulkan.hpp>

#include "../GLTFNode.hpp"
#include "../GLTFObject.hpp"

namespace pvk::gltf::loader::node {
    /**
     * Loads all nodes from the GLTF file.
     * @param model
     * @param primitiveLookup
     * @param graphicsQueue
     * @param object
     * @return Vector containing all nodes from the GLTF file.
     */
    std::vector<std::shared_ptr<pvk::gltf::Node>> loadNodes(
            const std::shared_ptr<tinygltf::Model> &model,
            const std::vector<std::vector<std::shared_ptr<pvk::gltf::Primitive>>> &primitiveLookup,
            vk::Queue &graphicsQueue,
            pvk::gltf::Object &object
    );
}  // namespace pvk::gltf::loader::node

#endif //PVK_GLTFLOADERNODE_HPP
