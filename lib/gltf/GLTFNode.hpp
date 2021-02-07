//
//  PvkGLTFNode.hpp
//  PVK
//
//  Created by Christian aan de Wiel on 10/01/2021.
//

#ifndef PvkGLTFNode_hpp
#define PvkGLTFNode_hpp

#include <cstdio>
#include <vector>
#include <map>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "GLTFPrimitive.hpp"
#include "../mesh/mesh.hpp"
#include "../buffer/buffer.hpp"

namespace pvk::gltf {
    class Node {
    public:
        Node();

        ~Node();

        glm::mat4 getLocalMatrix() const;

        std::vector<Node *> children{};
        std::vector<Primitive *> primitives{};
        Mesh *mesh{};
        Node *parent = nullptr;
        int skinIndex = -1;
        int nodeIndex = -1;
        std::string name;

        std::vector<vk::UniqueDescriptorSet> descriptorSets{};

        std::map<uint32_t, std::vector<vk::DescriptorBufferInfo>> descriptorBuffersInfo{};
        std::map<uint32_t, std::vector<vk::Buffer>> uniformBuffers{};
        std::map<uint32_t, std::vector<vk::DeviceMemory>> uniformBuffersMemory{};

        glm::vec3 translation = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
        glm::mat4 rotation = glm::mat4(1.0f);
        glm::mat4 matrix = glm::mat4(1.0f);
    };
}

#endif /* PvkGLTFNode_hpp */
