//  Heavily inspired on: https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanglTFModel.cpp
//  PvkGLTFLoader.cpp
//  PVK
//
//  Created by Christian aan de Wiel on 02/01/2021.
//

#include "GLTFLoader.hpp"

namespace pvk {
    inline bool endsWith(std::string const &value, std::string const &ending) {
        if (ending.size() > value.size()) return false;
        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    }

    gltf::Object *GLTFLoader::loadObject(vk::Queue &graphicsQueue,
                                         const std::string &filePath
    ) {
        tinygltf::TinyGLTF loader;
        tinygltf::Model model;
        std::string error;
        std::string warning;

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        bool isModelLoaded;

        if (endsWith(filePath, EXTENSION_GLB)) {
            isModelLoaded = loader.LoadBinaryFromFile(&model, &error, &warning, filePath);
        } else {
            isModelLoaded = loader.LoadASCIIFromFile(&model, &error, &warning, filePath);
        }

        if (!isModelLoaded) {
            throw std::runtime_error("Could not load glTF model");
        }

        auto object = new gltf::Object();
        object->nodes = GLTFLoader::loadNodes(model, graphicsQueue, object);
        object->nodeLookup = GLTFLoader::initializeNodeLookupTable(object->nodes);
        object->primitiveLookup = GLTFLoader::initializePrimitiveLookupTable(object->nodes);
        object->animations = GLTFLoader::loadAnimations(model, object->nodeLookup);

        pvk::buffer::vertex::create(graphicsQueue,
                                    object->vertexBuffer,
                                    object->vertexBufferMemory,
                                    object->vertices);

        if (!object->indices.empty()) {
            pvk::buffer::index::create(graphicsQueue,
                                       object->indexBuffer,
                                       object->indexBufferMemory,
                                       object->indices);
        }

        return object;
    }

    std::vector<gltf::Node *> GLTFLoader::loadNodes(tinygltf::Model &model,
                                                    vk::Queue &graphicsQueue,
                                                    gltf::Object* &object
    ) {
        const auto &scene = model.scenes[model.defaultScene];

        std::vector<gltf::Node *> nodes{};

        for (const auto &nodeIndex : scene.nodes) {
            auto _node = GLTFLoader::loadNode(model, nodeIndex, graphicsQueue, object);

            if (_node->children.empty() && _node->mesh == nullptr) {
                // Don't add this, most likely light or camera.
            } else {
                nodes.push_back(_node);
            }
        }

        return nodes;
    }

    gltf::Node *GLTFLoader::loadNode(tinygltf::Model &model,
                                     int nodeIndex,
                                     vk::Queue &graphicsQueue,
                                     gltf::Object *&object,
                                     gltf::Node *parent) {
        auto &node = model.nodes[nodeIndex];

        auto *resultNode = new gltf::Node();
        resultNode->parent = parent;
        resultNode->skinIndex = node.skin;
        resultNode->nodeIndex = nodeIndex;
        resultNode->matrix = glm::mat4(1.0f);
        resultNode->name = node.name;

        // First load all children recursively
        resultNode->children.reserve(node.children.size());

        for (auto &child : node.children) {
            resultNode->children.emplace_back(loadNode(model, child, graphicsQueue, object, resultNode));
        }

        if (node.translation.size() == 3) {
            resultNode->translation = glm::make_vec3(node.translation.data());
        }

        if (node.rotation.size() == 4) {
            glm::quat quaternion = glm::make_quat(node.rotation.data());
            resultNode->rotation = glm::mat4(quaternion);
        }

        if (node.scale.size() == 3) {
            resultNode->scale = glm::make_vec3(node.scale.data());
        }

        if (node.matrix.size() == 16) {
            resultNode->matrix = glm::make_mat4x4(node.matrix.data());
        } else {
            resultNode->matrix = glm::mat4(1.0f);
            auto translationMatrix = glm::translate(glm::mat4(1.0f), resultNode->translation);
            auto rotationMatrix = glm::mat4(resultNode->rotation);
            auto scaleMatrix = glm::scale(glm::mat4(1.0f), resultNode->scale);

            resultNode->matrix = translationMatrix * rotationMatrix * scaleMatrix * resultNode->matrix;
        }

        // Filter out all lights and cameras
        if (node.mesh == -1) {
            return resultNode;
        }

        resultNode->mesh = new Mesh();

        auto &mesh = model.meshes[node.mesh];

        for (auto &primitive : mesh.primitives) {
            auto startVertex = static_cast<uint32_t>(object->vertices.size());
            auto startIndex = static_cast<uint32_t>(object->indices.size());

            auto *resultPrimitive = new gltf::Primitive();

            const float *bufferPosition = GLTFLoader::loadVertexPositionBuffer(model, primitive);
            const float *bufferNormal = GLTFLoader::loadVertexNormalBuffer(model, primitive);
            const float *bufferColor = GLTFLoader::loadVertexColorBuffer(model, primitive);
            const float *bufferUV0 = GLTFLoader::loadVertexUV0Buffer(model, primitive);
            const float *bufferUV1 = GLTFLoader::loadVertexUV1Buffer(model, primitive);
            const uint8_t *bufferJoint = GLTFLoader::loadVertexJointBuffer(model, primitive);
            const float *bufferWeight = GLTFLoader::loadVertexWeightBuffer(model, primitive);

            const int positionByteStride = GLTFLoader::getVertexPositionByteStride(model, primitive);
            const int normalByteStride = GLTFLoader::getVertexNormalByteStride(model, primitive);
            const int jointByteStride = GLTFLoader::getVertexJointByteStride(model, primitive, bufferJoint);
            const int weightByteStride = GLTFLoader::getVertexWeightByteStride(model, primitive);
            const int colorType = GLTFLoader::getVertexColorType(model, primitive);

            for (size_t index = 0;
                 index < model.accessors[primitive.attributes.find("POSITION")->second].count; index++) {
                Vertex vertex{};

                GLTFLoader::loadVertexPosition(vertex, bufferPosition, positionByteStride, index);
                GLTFLoader::loadVertexNormal(vertex, bufferNormal, normalByteStride, index);
                GLTFLoader::loadVertexUV0(vertex, bufferUV0, index);
                GLTFLoader::loadVertexUV1(vertex, bufferUV1, index);
                GLTFLoader::loadVertexJoint(vertex, bufferJoint, jointByteStride, index);
                GLTFLoader::loadVertexWeight(vertex, bufferWeight, weightByteStride, index);
                GLTFLoader::loadVertexColor(vertex, bufferColor, colorType, index);

                object->vertices.push_back(vertex);
            }

            // Load indices
            GLTFLoader::loadIndices(object->indices, model, primitive, startVertex);
            resultPrimitive->startIndex = startIndex;
            resultPrimitive->startVertex = startVertex;
            resultPrimitive->indexCount = static_cast<uint32_t>(model.accessors[primitive.indices].count);
            resultPrimitive->vertexCount = static_cast<uint32_t>(model.accessors[primitive.attributes.find(
                    "POSITION")->second].count);
            GLTFLoader::loadMaterial(model, resultPrimitive, primitive.material);

            // Add primitive to node
            resultNode->primitives.push_back(resultPrimitive);
        }

        if (node.skin > -1) {
            auto &skin = model.skins[node.skin];
            auto _skin = new gltf::Skin();

            _skin->skinIndex = node.skin;
            _skin->jointsIndices.reserve(skin.joints.size());

            for (auto &joint : skin.joints) {
                _skin->jointsIndices.emplace_back(joint);
            }

            // Check whether there are inverse bind matrices present
            if (skin.inverseBindMatrices > -1) {
                const auto &accessor = model.accessors[skin.inverseBindMatrices];
                const auto &bufferView = model.bufferViews[accessor.bufferView];
                const auto &buffer = model.buffers[bufferView.buffer];

                _skin->skeletonRoot = resultNode;
                _skin->inverseBindMatrices.resize(accessor.count);
                memcpy(_skin->inverseBindMatrices.data(),
                       &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                       accessor.count * sizeof(glm::mat4));
            }

            object->skinLookup[node.skin] = _skin;
        }

        return resultNode;
    }

    const float *GLTFLoader::loadVertexPositionBuffer(const tinygltf::Model &model,
                                                      const tinygltf::Primitive &primitive) {
        assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

        const tinygltf::Accessor &positionAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
        const tinygltf::BufferView &positionView = model.bufferViews[positionAccessor.bufferView];

        return reinterpret_cast<const float *>(&(model.buffers[positionView.buffer].data[positionAccessor.byteOffset +
                                                                                         positionView.byteOffset]));
    }

    const float *GLTFLoader::loadVertexNormalBuffer(const tinygltf::Model &model,
                                                    const tinygltf::Primitive &primitive) {
        if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
            const tinygltf::Accessor &normalAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
            const tinygltf::BufferView &normalView = model.bufferViews[normalAccessor.bufferView];

            return reinterpret_cast<const float *>(&(model.buffers[normalView.buffer].data[normalAccessor.byteOffset +
                                                                                           normalView.byteOffset]));
        } else {
            return nullptr;
        }
    }

    const float *GLTFLoader::loadVertexColorBuffer(const tinygltf::Model &model,
                                                   const tinygltf::Primitive &primitive) {
        if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
            const tinygltf::Accessor &colorAccessor = model.accessors[primitive.attributes.find("COLOR_0")->second];
            const tinygltf::BufferView &colorView = model.bufferViews[colorAccessor.bufferView];

            return reinterpret_cast<const float *>(&(model.buffers[colorView.buffer].data[colorAccessor.byteOffset +
                                                                                          colorView.byteOffset]));
        } else {
            return nullptr;
        }
    }

    const float *GLTFLoader::loadVertexUV0Buffer(const tinygltf::Model &model,
                                                 const tinygltf::Primitive &primitive) {
        if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
            const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
            const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];

            return reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset +
                                                                                       uvView.byteOffset]));
        } else {
            return nullptr;
        }
    }

    const float *GLTFLoader::loadVertexUV1Buffer(const tinygltf::Model &model,
                                                 const tinygltf::Primitive &primitive) {
        if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end()) {
            const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
            const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];

            return reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset +
                                                                                       uvView.byteOffset]));
        } else {
            return nullptr;
        }
    }

    const uint8_t *GLTFLoader::loadVertexJointBuffer(const tinygltf::Model &model,
                                                      const tinygltf::Primitive &primitive) {
        if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
            const tinygltf::Accessor &jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
            const tinygltf::BufferView &jointView = model.bufferViews[jointAccessor.bufferView];

            return reinterpret_cast<const uint8_t *>(&(model.buffers[jointView.buffer].data[jointAccessor.byteOffset +
                                                                                             jointView.byteOffset]));
        } else {
            return nullptr;
        }
    }

    const float *GLTFLoader::loadVertexWeightBuffer(const tinygltf::Model &model,
                                                    const tinygltf::Primitive &primitive) {
        if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
            const tinygltf::Accessor &weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
            const tinygltf::BufferView &weightView = model.bufferViews[weightAccessor.bufferView];

            return reinterpret_cast<const float *>(&(model.buffers[weightView.buffer].data[weightAccessor.byteOffset +
                                                                                           weightView.byteOffset]));
        } else {
            return nullptr;
        }
    }

    int GLTFLoader::getVertexPositionByteStride(const tinygltf::Model &model, const tinygltf::Primitive &primitive) {
        assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

        const tinygltf::Accessor &positionAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
        const tinygltf::BufferView &positionView = model.bufferViews[positionAccessor.bufferView];

        if (positionAccessor.ByteStride(positionView)) {
            return static_cast<int>(positionAccessor.ByteStride(positionView) / sizeof(float));
        } else {
            return tinygltf::GetComponentSizeInBytes(TINYGLTF_TYPE_VEC3);
        }
    }

    int GLTFLoader::getVertexNormalByteStride(const tinygltf::Model &model, const tinygltf::Primitive &primitive) {
        if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
            const tinygltf::Accessor &normalAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
            const tinygltf::BufferView &normalView = model.bufferViews[normalAccessor.bufferView];

            if (normalAccessor.ByteStride(normalView)) {
                return static_cast<int>(normalAccessor.ByteStride(normalView) / sizeof(float));
            } else {
                return tinygltf::GetComponentSizeInBytes(TINYGLTF_TYPE_VEC3);
            }
        } else {
            return -1;
        }
    }

    int GLTFLoader::getVertexJointByteStride(const tinygltf::Model &model, const tinygltf::Primitive &primitive, const uint8_t * &buffer) {
        if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
            const tinygltf::Accessor &jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
            const tinygltf::BufferView &jointView = model.bufferViews[jointAccessor.bufferView];

            if (jointAccessor.ByteStride(jointView)) {
                return static_cast<int>(jointAccessor.ByteStride(jointView) / sizeof(buffer[0]));
            } else {
                return tinygltf::GetComponentSizeInBytes(TINYGLTF_TYPE_VEC4);
            }
        } else {
            return -1;
        }
    }

    int GLTFLoader::getVertexWeightByteStride(const tinygltf::Model &model, const tinygltf::Primitive &primitive) {
        if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
            const tinygltf::Accessor &weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
            const tinygltf::BufferView &weightView = model.bufferViews[weightAccessor.bufferView];

            if (weightAccessor.ByteStride(weightView)) {
                return static_cast<int>(weightAccessor.ByteStride(weightView) / sizeof(float));
            } else {
                return tinygltf::GetComponentSizeInBytes(TINYGLTF_TYPE_VEC4);
            }
        } else {
            return -1;
        }
    }

    int GLTFLoader::getVertexColorType(const tinygltf::Model &model, const tinygltf::Primitive &primitive) {
        if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
            return model.accessors[primitive.attributes.find("COLOR_0")->second].type;
        } else {
            return -1;
        }
    }

    void GLTFLoader::loadVertexPosition(Vertex &vertex, const float *bufferPosition, const int &positionByteStride,
                                        const size_t &index) {
        vertex.pos = glm::make_vec3(&bufferPosition[index * positionByteStride]);
    }

    void GLTFLoader::loadVertexNormal(Vertex &vertex, const float *bufferNormal, const int &normalByteStride,
                                      const size_t &index) {
        if (bufferNormal) {
            const float *normal = &bufferNormal[index * normalByteStride];

            vertex.normal = glm::normalize(glm::vec3(normal[0], normal[1], normal[2]));
        } else {
            vertex.normal = glm::vec3(0.0f);
        }
    }

    void GLTFLoader::loadVertexUV0(Vertex &vertex, const float *bufferTexCoord, const size_t &index) {
        if (bufferTexCoord) {
            const float *uv = &bufferTexCoord[index * 2];

            vertex.UV0 = glm::vec2(uv[0], uv[1]);
        } else {
            vertex.UV0 = glm::vec2(0.0f);
        }
    }

    void GLTFLoader::loadVertexUV1(Vertex &vertex, const float *bufferTexCoord, const size_t &index) {
        if (bufferTexCoord) {
            const float *uv = &bufferTexCoord[index * 2];

            vertex.UV1 = glm::vec2(uv[0], uv[1]);
        } else {
            vertex.UV1 = glm::vec2(0.0f);
        }
    }

    void GLTFLoader::loadVertexJoint(Vertex &vertex,
                                     const uint8_t *bufferJoint,
                                     const int &jointByteStride,
                                     const size_t &index) {
        if (bufferJoint) {
            const uint8_t *joint = &bufferJoint[index * jointByteStride];
            vertex.joint = glm::ivec4(joint[0], joint[1], joint[2], joint[3]);
        } else {
            vertex.joint = glm::vec4(0.0f);
        }
    }

    void GLTFLoader::loadVertexWeight(Vertex &vertex,
                                      const float *bufferWeight,
                                      const int &weightByteStride,
                                      const size_t &index) {
        if (bufferWeight) {
            const float *weight = &bufferWeight[index * weightByteStride];
            vertex.weight = glm::make_vec4(weight);
        } else {
            vertex.weight = glm::vec4(0.0f);
        }
    }

    void GLTFLoader::loadVertexColor(Vertex &vertex, const float *bufferColor, const int &type, const size_t &index) {
        switch (type) {
            case TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3: {
                const float *color = &bufferColor[index * 3];
                vertex.color = glm::vec3(color[0], color[1], color[2]);

                break;
            }
            case TINYGLTF_PARAMETER_TYPE_FLOAT_VEC4: {
                const float *color = &bufferColor[index * 4];
                vertex.color = glm::vec3(color[0], color[1], color[2]);
            }
            default:
                vertex.color = glm::vec3(1.0f);
                break;
        }
    }

    void GLTFLoader::loadIndices(std::vector<uint32_t> &indices,
                                 tinygltf::Model &model,
                                 tinygltf::Primitive &primitive,
                                 uint32_t &vertexStart) {
        if (primitive.indices == -1) {
            // Model has no indices.
            return;
        }

        const tinygltf::Accessor &indexAccessor = model.accessors[primitive.indices];
        const tinygltf::BufferView &indexBufferView = model.bufferViews[indexAccessor.bufferView];
        const tinygltf::Buffer &indexBuffer = model.buffers[indexBufferView.buffer];

        switch (indexAccessor.componentType) {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                auto *buffer = new uint32_t[indexAccessor.count];
                memcpy(buffer, &indexBuffer.data[indexAccessor.byteOffset + indexBufferView.byteOffset],
                       sizeof(uint32_t) * indexAccessor.count);
                for (size_t index = 0; index < indexAccessor.count; index++) {
                    indices.push_back(buffer[index] + vertexStart);
                }
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                auto *buffer = new uint16_t[indexAccessor.count];
                memcpy(buffer, &indexBuffer.data[indexAccessor.byteOffset + indexBufferView.byteOffset], sizeof(uint16_t) * indexAccessor.count);
                for (size_t index = 0; index < indexAccessor.count; index++) {
                    indices.push_back(buffer[index] + vertexStart);
                }
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                auto *buffer = new uint8_t[indexAccessor.count];
                memcpy(buffer, &indexBuffer.data[indexAccessor.byteOffset + indexBufferView.byteOffset],
                       sizeof(uint8_t) * indexAccessor.count);
                for (size_t index = 0; index < indexAccessor.count; index++) {
                    indices.push_back(static_cast<uint32_t>(buffer[index]) + vertexStart);
                }
                break;
            }
            default: {
                throw std::runtime_error("Unsupported glTF index component type");
            }
        }
    }

    void GLTFLoader::loadMaterial(tinygltf::Model &model,
                                  gltf::Primitive *primitive,
                                  uint32_t materialIndex) {
        auto material = model.materials[materialIndex];

        auto &baseColorFactor = material.pbrMetallicRoughness.baseColorFactor;
        primitive->material.baseColorFactor = glm::vec4(baseColorFactor[0], baseColorFactor[1], baseColorFactor[2],
                                                        baseColorFactor[3]);
        primitive->material.metallicFactor = static_cast<float>(material.pbrMetallicRoughness.metallicFactor);
        primitive->material.roughnessFactor = static_cast<float>(material.pbrMetallicRoughness.roughnessFactor);
    }

    std::vector<gltf::Animation *> GLTFLoader::loadAnimations(tinygltf::Model &model,
                                                              std::map<uint32_t, gltf::Node *> &nodeLookup) {
        auto loadAnimationInputs = [&model](tinygltf::AnimationSampler &sampler, gltf::Sampler &_sampler) {
            auto &accessor = model.accessors[sampler.input];
            auto &bufferView = model.bufferViews[accessor.bufferView];
            auto &buffer = model.buffers[bufferView.buffer];

            const void *bufferPointer = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
            const auto *timeValueBuffer = static_cast<const float *>(bufferPointer);

            if (sampler.interpolation == "LINEAR") {
                _sampler.interpolationType = gltf::Sampler::InterpolationType::LINEAR;
            } else if (sampler.interpolation == "STEP") {
                _sampler.interpolationType = gltf::Sampler::InterpolationType::STEP;
            } else if (sampler.interpolation == "CUBICSPLINE") {
                _sampler.interpolationType = gltf::Sampler::InterpolationType::CUBICSPLINE;
            } else {
                throw std::runtime_error("Unsupported animation interpolation type");
            }

            _sampler.inputs.reserve(accessor.count);

            for (size_t i = 0; i < accessor.count; i++) {
                _sampler.inputs.emplace_back(timeValueBuffer[i]);
            }
        };

        auto loadAnimationOutputs = [&model](tinygltf::AnimationSampler &sampler, gltf::Sampler &_sampler) {
            auto &accessor = model.accessors[sampler.output];
            auto &bufferView = model.bufferViews[accessor.bufferView];
            auto &buffer = model.buffers[bufferView.buffer];

            const void *bufferPointer = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
            _sampler.outputs.reserve(accessor.count);

            switch (accessor.type) {
                case TINYGLTF_TYPE_VEC3: {
                    const auto *outputsPointer = static_cast<const glm::vec3 *>(bufferPointer);

                    for (size_t i = 0; i < accessor.count; i++) {
                        _sampler.outputs.emplace_back(glm::vec4(outputsPointer[i], 0.0f));
                    }

                    break;
                }
                case TINYGLTF_TYPE_VEC4: {
                    const auto *outputsPointer = static_cast<const glm::vec4 *>(bufferPointer);

                    for (size_t i = 0; i < accessor.count; i++) {
                        _sampler.outputs.emplace_back(outputsPointer[i]);
                    }

                    break;
                }
                case TINYGLTF_TYPE_SCALAR: {

                    break;
                }
                default:
                    throw std::runtime_error("Unsupported animation output type");
            }
        };

        auto loadAnimationChannels = [&nodeLookup](tinygltf::AnimationChannel &channel, gltf::Channel &_channel) {
            if (channel.target_path == "rotation") {
                _channel.pathType = gltf::Channel::PathType::ROTATION;
            } else if (channel.target_path == "translation") {
                _channel.pathType = gltf::Channel::PathType::TRANSLATION;
            } else if (channel.target_path == "scale") {
                _channel.pathType = gltf::Channel::PathType::SCALE;
            } else {
                // @TODO: Implement weights later.
            }

            _channel.samplerIndex = channel.sampler;
            _channel.node = nodeLookup[channel.target_node];
        };

        std::vector<gltf::Animation *> animations;
        animations.reserve(model.animations.size());

        for (auto &animation : model.animations) {
            auto _animation = new gltf::Animation();
            _animation->samplers.reserve(animation.samplers.size());
            _animation->channels.reserve(animation.channels.size());

            for (auto &sampler : animation.samplers) {
                gltf::Sampler localSampler;

                loadAnimationInputs(sampler, localSampler);
                loadAnimationOutputs(sampler, localSampler);

                _animation->samplers.emplace_back(localSampler);
            }

            for (auto &channel : animation.channels) {
                gltf::Channel localChannel{};

                loadAnimationChannels(channel, localChannel);

                _animation->channels.emplace_back(localChannel);
            }

            _animation->currentTime = 0.0f;
            _animation->startTime = 0.0f;
            _animation->endTime = -1.0f;

            for (auto &sampler : _animation->samplers) {
                for (auto &input : sampler.inputs) {
                    if (_animation->endTime == -1.0f) {
                        _animation->endTime = input;
                    } else if (input > _animation->endTime) {
                        _animation->endTime = input;
                    }
                }
            }

            animations.emplace_back(_animation);
        }

        return animations;
    }

    std::vector<gltf::Skin *> GLTFLoader::loadSkins(tinygltf::Model &model,
                                                    std::map<uint32_t, gltf::Node *> &nodeLookup) {
        std::vector<gltf::Skin *> allSkins;
        allSkins.reserve(model.skins.size());

        for (auto &skin : model.skins) {
            auto _skin = new gltf::Skin();

            _skin->skeletonRoot = nodeLookup[skin.skeleton];
            _skin->jointsIndices.reserve(skin.joints.size());

            for (auto &joint : skin.joints) {
                _skin->jointsIndices.emplace_back(joint);
            }

            // Check whether there are inverse bind matrices present
            if (skin.inverseBindMatrices > -1) {
                const auto &accessor = model.accessors[skin.inverseBindMatrices];
                const auto &bufferView = model.bufferViews[accessor.bufferView];
                const auto &buffer = model.buffers[bufferView.buffer];

                _skin->inverseBindMatrices.resize(accessor.count);
                memcpy(_skin->inverseBindMatrices.data(),
                       &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                       accessor.count * sizeof(glm::mat4));
            }

            allSkins.emplace_back(_skin);
        }

        return allSkins;
    }

    inline std::vector<gltf::Node *> getAllNode(gltf::Node *node) {
        std::vector<gltf::Node *> allNodes;
        allNodes.reserve(1);
        allNodes.emplace_back(node);

        for (auto &child : node->children) {
            auto allChildrenNodes = getAllNode(child);
            allNodes.reserve(allNodes.size() + allChildrenNodes.size());
            allNodes.insert(allNodes.end(), allChildrenNodes.begin(), allChildrenNodes.end());
        }

        return allNodes;
    }

    std::map<uint32_t, gltf::Node *> GLTFLoader::initializeNodeLookupTable(std::vector<gltf::Node *> &nodes) {
        std::map<uint32_t, gltf::Node *> nodeLookup{};

        for (auto &rootNode : nodes) {
            auto allNodes = getAllNode(rootNode);

            for (auto &node : allNodes) {
                nodeLookup[node->nodeIndex] = node;
            }
        }

        return nodeLookup;
    }

    inline std::vector<gltf::Primitive *> getAllPrimitive(gltf::Node *node) {
        std::vector<gltf::Primitive *> allPrimitive;

        if (!node->primitives.empty()) {
            allPrimitive.reserve(allPrimitive.size() + node->primitives.size());
            allPrimitive.insert(allPrimitive.end(), node->primitives.begin(), node->primitives.end());
        };

        for (auto &child : node->children) {
            auto allPrimitiveChildren = getAllPrimitive(child);
            allPrimitive.reserve(allPrimitive.size() + allPrimitiveChildren.size());
            allPrimitive.insert(allPrimitive.end(), allPrimitiveChildren.begin(), allPrimitiveChildren.end());
        }

        return allPrimitive;
    }

    std::map<uint32_t, std::vector<gltf::Primitive *>> GLTFLoader::initializePrimitiveLookupTable(std::vector<gltf::Node *> &nodes) {
        std::map<uint32_t, std::vector<gltf::Primitive *>> primitiveLookup;

        for (auto &node : nodes) {
            auto allPrimitive = getAllPrimitive(node);
            primitiveLookup[node->nodeIndex] = allPrimitive;
        }

        return primitiveLookup;
    }
}
