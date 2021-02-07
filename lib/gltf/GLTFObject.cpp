//
//  PvkGLTFObject.cpp
//  PVK
//
//  Created by Christian aan de Wiel on 10/01/2021.
//

#include "GLTFObject.hpp"

#include <utility>

namespace pvk::gltf {
    Object::Object(std::vector<Node *> nodes,
                   std::map<uint32_t, Node *> nodeLookup,
                   std::map<uint32_t, std::vector<Primitive *>> primitiveLookup,
                   std::vector<Animation *> animations,
                   std::vector<Skin *> skins) {
        this->nodes = std::move(nodes);
        this->nodeLookup = std::move(nodeLookup);
        this->primitiveLookup = std::move(primitiveLookup);
        this->animations = std::move(animations);
        this->skins = std::move(skins);
    }

    Object::~Object() = default;

    void Object::initializeDescriptorSets(const vk::Device &logicalDevice,
                                          const vk::DescriptorPool &descriptorPool,
                                          const vk::DescriptorSetLayout &descriptorSetLayout,
                                          const uint32_t numberOfSwapChainImages) {
        std::vector<vk::DescriptorSetLayout> layouts(numberOfSwapChainImages, descriptorSetLayout);

        for (auto &node : this->nodes) {
            vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
            descriptorSetAllocateInfo.descriptorPool = descriptorPool;
            descriptorSetAllocateInfo.descriptorSetCount = numberOfSwapChainImages;
            descriptorSetAllocateInfo.pSetLayouts = layouts.data();

            node->descriptorSets.resize(numberOfSwapChainImages);
            node->descriptorSets = logicalDevice.allocateDescriptorSetsUnique(descriptorSetAllocateInfo);
        }
    }

    void Object::updateJoints() {
        this->updateJointsByNode(this->nodes[0]);
    }

void Object::updateJointsByNode(Node *node) {
    if (node->mesh && node->skinIndex > -1) {
        auto inverseTransform = glm::inverse(node->getLocalMatrix());
        auto skin = this->skinLookup[node->skinIndex];
        auto numberOfJoints = skin->jointsIndices.size();

        std::vector<glm::mat4> jointMatrices(numberOfJoints);

        for (size_t i = 0; i < numberOfJoints; i++) {
            jointMatrices[i] =
                    this->getNodeByIndex(skin->jointsIndices[i])->getLocalMatrix() * skin->inverseBindMatrices[i];
            jointMatrices[i] = inverseTransform * jointMatrices[i];
        }

        this->inverseBindMatrices = jointMatrices;
    }

    for (auto &child : node->children) {
        this->updateJointsByNode(child);
    }
}

    Node *Object::getNodeByIndex(uint32_t index) {
        return nodeLookup[index];
    }
}
