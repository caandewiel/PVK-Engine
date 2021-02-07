//
// Created by Christian aan de Wiel on 04/02/2021.
//

#ifndef PVK_IO_HPP
#define PVK_IO_HPP

#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vulkan/vulkan.hpp>
#include <map>
#include <unordered_map>
#include <glm/gtc/type_ptr.hpp>

#include "../buffer/buffer.hpp"
#include "../mesh/mesh.hpp"
#include "../mesh/vertex.hpp"
#include "../context/context.hpp"

namespace pvk {
    struct AssimpNode {
        std::string name;
        uint32_t index;
        AssimpNode *parent;
        glm::mat4 localMatrix;
        std::vector<AssimpNode *> children;
        std::vector<uint32_t> meshIndices;

        glm::mat4 getLocalMatrix() const;
    };

    struct AssimpMesh {
        uint32_t startIndex;
        uint32_t startVertex;
        uint32_t indexCount;
        uint32_t vertexCount;

        std::vector<vk::UniqueDescriptorSet> descriptorSets;

        std::map<uint32_t, std::vector<vk::DescriptorBufferInfo>> descriptorBuffersInfo;
        std::map<uint32_t, std::vector<vk::Buffer>> uniformBuffers;
        std::map<uint32_t, std::vector<vk::DeviceMemory>> uniformBuffersMemory;
    };

    struct AssimpBone {
        std::string name;
        uint32_t index;
        glm::mat4 inverseBindMatrix;
        std::vector<AssimpBone*> children;
    };

    struct AssimpObject {
        AssimpNode *rootNode;
        AssimpBone *skeleton;
        std::unordered_map<std::string, AssimpNode*> nodeLookup;
        std::unordered_map<std::string, AssimpBone*> boneLookup;

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        std::map<uint32_t, AssimpMesh *> meshLookup;

        glm::mat4 globalInverseTransform;

        std::vector<glm::mat4> jointMatrices;

        vk::Buffer vertexBuffer;
        vk::DeviceMemory vertexBufferMemory;
        vk::Buffer indexBuffer;
        vk::DeviceMemory indexBufferMemory;

        void initializeDescriptorSets(const vk::DescriptorPool &descriptorPool,
                                      const vk::DescriptorSetLayout &descriptorSetLayout,
                                      uint32_t numberOfSwapChainImages);

        void updateUniformBuffer(uint32_t bindingIndex, size_t size, void *data) const;

        void updateUniformBufferPerNode(uint32_t bindingIndex,
                                        const std::function<void(pvk::AssimpObject *object,
                                                                 pvk::AssimpNode *node,
                                                                 vk::DeviceMemory &memory)> &function);

        void updatePose();

        void updatePoseByNode(AssimpNode *node);

        AssimpNode* getNodeOrNull(std::string &name);
        AssimpBone* getBoneOrNull(std::string &name);
    };

    glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4 &from);

    AssimpNode *loadNode(AssimpObject *object, aiNode *node, AssimpNode *parent = nullptr, uint32_t index = 0);

    bool loadSkeleton(AssimpObject *object, AssimpBone *bone, AssimpNode *node);

    AssimpObject *loadModel(const std::string &filename, vk::Queue &graphicsQueue);
}

#endif //PVK_IO_HPP
