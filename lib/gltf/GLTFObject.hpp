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
        Object(std::vector<Node *> nodes,
               std::map<uint32_t, Node *> nodeLookup,
               std::map<uint32_t, std::vector<Primitive *>> primitiveLookup,
               std::vector<Animation *> animations,
               std::vector<Skin *> skins);

        ~Object();

        std::vector<Node *> nodes;
        std::map<uint32_t, Node *> nodeLookup;
        std::map<uint32_t, std::vector<Primitive *>> primitiveLookup;
        std::map<uint32_t, Skin *> skinLookup;
        std::vector<Animation *> animations;
        std::vector<Skin *> skins;
        std::vector<glm::mat4> inverseBindMatrices;
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
        std::vector<Material*> materials{};
        vk::Buffer vertexBuffer{};
        vk::DeviceMemory vertexBufferMemory{};
        vk::Buffer indexBuffer{};
        vk::DeviceMemory indexBufferMemory{};

        void initializeDescriptorSets(const vk::Device &logicalDevice,
                                      const vk::DescriptorPool &descriptorPool,
                                      const vk::DescriptorSetLayout &descriptorSetLayout,
                                      uint32_t numberOfSwapChainImages);

        void updateJoints();
        void updateJointsByNode(Node* node);

        Node *getNodeByIndex(uint32_t index);
    };
}

#endif /* PvkGLTFObject_hpp */
