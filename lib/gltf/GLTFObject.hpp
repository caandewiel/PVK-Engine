//
//  PvkGLTFObject.hpp
//  PVK
//
//  Created by Christian aan de Wiel on 10/01/2021.
//

#ifndef PvkGLTFObject_hpp
#define PvkGLTFObject_hpp

#include <cstdio>
#include <vector>
#include <map>
#include <vulkan/vulkan.hpp>

#include "GLTFNode.hpp"
#include "GLTFAnimation.hpp"
#include "GLTFSkin.hpp"
#include "GLTFMaterial.hpp"

namespace pvk::gltf {
    class Object {
    public:
        Object() = default;;
        Object(std::vector<std::shared_ptr<Node>> nodes,
               std::map<uint32_t, std::shared_ptr<Node>> nodeLookup,
               std::map<uint32_t, std::vector<Primitive *>> primitiveLookup,
               std::vector<std::unique_ptr<Animation>> animations,
               std::vector<std::shared_ptr<Skin>> skins);

        ~Object();

        std::vector<std::shared_ptr<Node>> nodes;
        std::map<uint32_t, std::shared_ptr<Node>> nodeLookup;
        std::map<uint32_t, std::vector<Primitive *>> primitiveLookup;
        std::map<uint32_t, std::shared_ptr<Skin>> skinLookup;
        std::vector<std::unique_ptr<Animation>> animations;
        std::vector<std::shared_ptr<Skin>> skins;
        std::vector<glm::mat4> inverseBindMatrices;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<Material*> materials;
        vk::Buffer vertexBuffer;
        vk::DeviceMemory vertexBufferMemory;
        vk::Buffer indexBuffer;
        vk::DeviceMemory indexBufferMemory;

        void initializeWriteDescriptorSets(const vk::Device &logicalDevice,
                                           const vk::DescriptorPool &descriptorPool,
                                           const vk::DescriptorSetLayout &descriptorSetLayout,
                                           uint32_t numberOfSwapChainImages);

        void updateJoints();

        void updateJointsByNode(Node &node);

        [[nodiscard]] std::shared_ptr<const Node> getNodeByIndex(uint32_t index) const {
            auto it = nodeLookup.find(index);

            return it == nodeLookup.end() ? nullptr : it->second;
        }

        [[nodiscard]] std::shared_ptr<Node> getNodeByIndex(uint32_t index) {
            auto it = nodeLookup.find(index);

            return it == nodeLookup.end() ? nullptr : it->second;
        }
    };
}

#endif /* PvkGLTFObject_hpp */
