//
// Created by Christian aan de Wiel on 05/02/2021.
//
#include "io.hpp"

namespace pvk {
    /**
     * https://lechior.blogspot.com/2017/05/skeletal-animation-using-assimp-opengl.html
     * @param from
     * @return
     */
    glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4 &from) {
        glm::mat4 m;
        for (int y = 0; y < 4; y++)
        {
            for (int x = 0; x < 4; x++)
            {
                m[x][y] = from[y][x];
            }
        }
        return m;
    }

    AssimpNode *loadNode(AssimpObject *object, aiNode *node, AssimpNode *parent, uint32_t index) {
        auto _node = new AssimpNode();
        std::string nodeName (node->mName.data);
        _node->name = nodeName;
        _node->index = index;
        _node->children.reserve(node->mNumChildren);
        _node->meshIndices.reserve(node->mNumMeshes);
        _node->parent = parent;
        _node->localMatrix = aiMatrix4x4ToGlm(node->mTransformation);

        for (size_t i = 0; i < node->mNumChildren; i++) {
            _node->children.emplace_back(loadNode(object, node->mChildren[i], _node, index++));
        }

        for (size_t i = 0; i < node->mNumMeshes; i++) {
            _node->meshIndices.emplace_back(node->mMeshes[i]);
        }

        object->nodeLookup[nodeName] = _node;

        return _node;
    }

    bool loadSkeleton(AssimpObject *object, AssimpBone *bone, AssimpNode *node) {
        auto _bone = object->getBoneOrNull(node->name);

        if (_bone != nullptr) {
            if (bone->name.empty()) {
                object->skeleton = _bone;
                loadSkeleton(object, _bone, node);
            }

            for (auto &childNode : node->children) {
                auto childBone = object->getBoneOrNull(childNode->name);
                if (childBone) {
                    loadSkeleton(object, childBone, childNode);
                    bone->children.push_back(childBone);
                }
            }

            return true;
        } else {
            for (auto &child : node->children) {
                if (loadSkeleton(object, bone, child)) {
                    return true;
                }
            }
        }

        return false;
    }

    AssimpObject *loadModel(const std::string &filename, vk::Queue &graphicsQueue) {
        Assimp::Importer importer;

        const auto scene = importer.ReadFile(filename, aiProcess_LimitBoneWeights | aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals);

        if (!scene) {
            throw std::runtime_error("File load failed.");
        }

        auto object = new AssimpObject();
        object->rootNode = loadNode(object, scene->mRootNode);

        auto _globalInverseTransform = scene->mRootNode->mTransformation;
        object->globalInverseTransform = aiMatrix4x4ToGlm(_globalInverseTransform.Inverse());

        for (size_t i = 0; i < scene->mNumMeshes; i++) {
            auto &mesh = scene->mMeshes[i];
            auto _mesh = new AssimpMesh();
            _mesh->startVertex = static_cast<uint32_t>(object->vertices.size());
            _mesh->startIndex = static_cast<uint32_t>(object->indices.size());

            object->vertices.reserve(object->vertices.size() + mesh->mNumVertices);

            for (size_t j = 0; j < mesh->mNumVertices; j++) {
                auto &vertex = mesh->mVertices[j];
                auto _vertex = Vertex{};
                _vertex.pos = glm::vec3(vertex.x, vertex.y, vertex.z);

                if (mesh->HasNormals()) {
                    auto &normal = mesh->mNormals[j];
                    _vertex.normal = glm::vec3(normal.x, normal.y, normal.z);
                }

                if (mesh->HasVertexColors(j)) {
                    auto &color = mesh->mColors[j];
                    _vertex.color = glm::vec3(color->r, color->g, color->b);
                }

                object->vertices.emplace_back(_vertex);
                _mesh->vertexCount++;
            }

            if (mesh->HasBones()) {
                for (size_t j = 0; j < mesh->mNumBones; j++) {
                    auto &bone = mesh->mBones[j];
                    std::string boneName (bone->mName.data);

                    auto _bone = new AssimpBone();
                    _bone->name = boneName;
                    _bone->inverseBindMatrix = aiMatrix4x4ToGlm(bone->mOffsetMatrix);
                    _bone->index = j;

                    for (size_t k = 0; k < bone->mNumWeights; k++) {
                        auto &weight = bone->mWeights[k];
                        auto &vertex = object->vertices[weight.mVertexId];

                        if (vertex.weight.x == 0) {
                            vertex.weight.x = weight.mWeight;
                            vertex.joint.x = j;
                        } else if (vertex.weight.y == 0) {
                            vertex.weight.y = weight.mWeight;
                            vertex.joint.y = j;
                        } else if (vertex.weight.z == 0) {
                            vertex.weight.z = weight.mWeight;
                            vertex.joint.z = j;
                        } else if (vertex.weight.w == 0) {
                            vertex.weight.w = weight.mWeight;
                            vertex.joint.w = j;
                        } else {
                            // Do not add this weight, since a vertex can have max 4 weights
                        }

//                        vertex.currentJoint++;
                    }

                    object->boneLookup[boneName] = _bone;
                }
            }

            for (auto &vertex : object->vertices) {
                vertex.weight = vertex.weight / (vertex.weight.x + vertex.weight.y + vertex.weight.z + vertex.weight.w);
            }

            auto skeleton = new AssimpBone();
            skeleton->inverseBindMatrix = glm::mat4(1.0f);
            loadSkeleton(object, skeleton, object->rootNode);

            if (mesh->HasFaces()) {
                object->indices.reserve(object->indices.size() + mesh->mNumFaces * 3);
                for (size_t j = 0; j < mesh->mNumFaces; j++) {
                    auto &face = mesh->mFaces[j];
                    assert(face.mNumIndices == 3);

                    for (size_t k = 0; k < face.mNumIndices; k++) {
                        object->indices.emplace_back(face.mIndices[k]);
                        _mesh->indexCount++;
                    }
                }
            }

            object->meshLookup[i] = _mesh;
        }

        pvk::buffer::vertex::create(graphicsQueue, object->vertexBuffer, object->vertexBufferMemory, object->vertices);
        pvk::buffer::index::create(graphicsQueue, object->indexBuffer, object->indexBufferMemory, object->indices);

        return object;
    }

    void AssimpObject::initializeDescriptorSets(const vk::DescriptorPool &descriptorPool,
                                                const vk::DescriptorSetLayout &descriptorSetLayout,
                                                const uint32_t numberOfSwapChainImages) {

        std::vector<vk::DescriptorSetLayout> layouts(numberOfSwapChainImages, descriptorSetLayout);

        for (auto &mesh : this->meshLookup) {
            vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
            descriptorSetAllocateInfo.descriptorPool = descriptorPool;
            descriptorSetAllocateInfo.descriptorSetCount = numberOfSwapChainImages;
            descriptorSetAllocateInfo.pSetLayouts = layouts.data();

            mesh.second->descriptorSets.resize(numberOfSwapChainImages);
            mesh.second->descriptorSets = Context::getLogicalDevice().allocateDescriptorSetsUnique(
                    descriptorSetAllocateInfo);
        }
    }

    void AssimpObject::updateUniformBuffer(uint32_t bindingIndex, size_t size, void *data) const {
        for (auto &mesh : this->meshLookup) {
            auto &uniformBuffersMemory = mesh.second->uniformBuffersMemory[bindingIndex];

            for (auto &uniformBufferMemory : uniformBuffersMemory) {
                pvk::buffer::update(uniformBufferMemory, size, data);
            }
        }
    }

    void AssimpObject::updateUniformBufferPerNode(uint32_t bindingIndex,
                                                  const std::function<void(pvk::AssimpObject *object,
                                                                     pvk::AssimpNode *node,
                                                                     vk::DeviceMemory &memory)> &function) {
        for (auto &node : this->nodeLookup) {
            for (auto &meshIndex : node.second->meshIndices) {
                auto &mesh = this->meshLookup[meshIndex];
                auto &uniformBuffersMemory = mesh->uniformBuffersMemory[bindingIndex];

                for (auto &uniformBufferMemory : uniformBuffersMemory) {
                    function(this, node.second, uniformBufferMemory);
                }
            }
        }
    }

    glm::mat4 AssimpNode::getLocalMatrix() const {
        auto matrix = this->localMatrix;
        auto currentParent = this->parent;

        while (currentParent) {
            matrix = currentParent->localMatrix * matrix;
            currentParent = currentParent->parent;
        }

        return localMatrix;
    }

    void AssimpObject::updatePoseByNode(AssimpNode *node) {
        if (!node->meshIndices.empty()) {
            auto inverseTransform = glm::inverse(node->getLocalMatrix());

            for (auto &bone : this->boneLookup) {
                auto nodeName = bone.first;
                auto globalTransform = this->getNodeOrNull(nodeName)->getLocalMatrix() * bone.second->inverseBindMatrix;
                this->jointMatrices[bone.second->index] = inverseTransform * globalTransform;
            }
        }

        for (auto &child : node->children) {
            this->updatePoseByNode(child);
        }
    }

    void AssimpObject::updatePose() {
        this->jointMatrices.resize(this->boneLookup.size());
        this->updatePoseByNode(this->rootNode);
    }

    AssimpNode *AssimpObject::getNodeOrNull(std::string &name) {
        if (this->nodeLookup.find(name) == this->nodeLookup.end()) {
            return nullptr;
        } else {
            return this->nodeLookup.find(name)->second;
        }
    }

    AssimpBone *AssimpObject::getBoneOrNull(std::string &name) {
        if (this->boneLookup.find(name) == this->boneLookup.end()) {
            return nullptr;
        } else {
            return this->boneLookup.find(name)->second;
        }
    }
}