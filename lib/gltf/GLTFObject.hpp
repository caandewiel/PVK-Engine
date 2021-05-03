//
//  PvkGLTFObject.hpp
//  PVK
//
//  Created by Christian aan de Wiel on 10/01/2021.
//

#ifndef PvkGLTFObject_hpp
#define PvkGLTFObject_hpp


#include <vector>
#include <map>
#include <vulkan/vulkan.hpp>
#include <boost/container/flat_map.hpp>

#include "GLTFNode.hpp"
#include "GLTFAnimation.hpp"
#include "GLTFSkin.hpp"
#include "GLTFMaterial.hpp"

namespace pvk::gltf {
    class Object {
    public:
        Object() = default;;

        Object(std::vector<std::shared_ptr<Node>> nodes,
               boost::container::flat_map<uint32_t, std::weak_ptr<Node>> nodeLookup,
               std::map<uint32_t, std::vector<std::weak_ptr<Primitive>>> primitiveLookup,
               std::vector<std::unique_ptr<Animation>> animations,
               std::vector<std::shared_ptr<Skin>> skins);

        ~Object();

        std::vector<std::shared_ptr<Node>> nodes;
        std::map<uint32_t, std::vector<std::weak_ptr<Primitive>>> primitiveLookup;
        std::map<uint32_t, std::shared_ptr<Skin>> skinLookup;
        std::vector<std::unique_ptr<Animation>> animations;
        std::vector<std::shared_ptr<Skin>> skins;
        std::vector<glm::mat4> inverseBindMatrices;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<std::unique_ptr<gltf::Material>> materials;
        vk::UniqueBuffer vertexBuffer;
        vk::UniqueDeviceMemory vertexBufferMemory;
        vk::UniqueBuffer indexBuffer;
        vk::UniqueDeviceMemory indexBufferMemory;

        void initializeWriteDescriptorSets(const vk::DescriptorPool &descriptorPool,
                                           const vk::DescriptorSetLayout &descriptorSetLayout,
                                           uint32_t numberOfSwapChainImages,
                                           uint32_t descriptorSetIndex);

        void updateJoints();

        void updateJointsByNode(Node &node);

        [[nodiscard]] const Node & getNodeByIndex(uint32_t index) const {
            auto it = nodeLookup.find(index);

            if (it == nodeLookup.end()) {
                throw std::runtime_error("Node not found by index.");
            } else {
                return *it->second.lock();
            }
        }

        [[nodiscard]] size_t getNumberOfPrimitives() const {
            size_t numberOfPrimitives = 0;

            for (const auto &primitivesByNode : this->primitiveLookup) {
                numberOfPrimitives += primitivesByNode.second.size();
            }

            return numberOfPrimitives;
        }

        [[nodiscard]] const boost::container::flat_map<uint32_t, std::weak_ptr<Node>> &getNodes() const {
            return this->nodeLookup;
        }

        void setNodeLookup(boost::container::flat_map<uint32_t, std::weak_ptr<Node>> newNodeLookup) {
            this->nodeLookup = std::move(newNodeLookup);
        }

    private:
        boost::container::flat_map<uint32_t, std::weak_ptr<Node>> nodeLookup;
    };
}  // namespace pvk::gltf

#endif /* PvkGLTFObject_hpp */
