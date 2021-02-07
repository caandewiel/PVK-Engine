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
                                                    gltf::Object *&object
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

            for (size_t index = 0;
                 index < model.accessors[primitive.attributes.find("POSITION")->second].count; index++) {
                Vertex vertex{};

                GLTFLoader::loadVertexPosition(model, primitive, vertex, index);
                GLTFLoader::loadVertexNormal(model, primitive, vertex, index);
                GLTFLoader::loadVertexUV0(model, primitive, vertex, index);
                GLTFLoader::loadVertexUV1(model, primitive, vertex, index);
                GLTFLoader::loadVertexJoints(model, primitive, vertex, index);
                GLTFLoader::loadVertexWeights(model, primitive, vertex, index);
                GLTFLoader::loadVertexColor(model, primitive, vertex, index);

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

    void GLTFLoader::loadVertexPosition(const tinygltf::Model &model,
                                        const tinygltf::Primitive &primitive,
                                        Vertex &vertex,
                                        const uint32_t index) {
        // The position attribute is mandatory.
        const auto &positionAccessor = model.accessors[primitive.attributes.find(FIELD_VERTEX_POSITION)->second];
        const auto &positionView = model.bufferViews[positionAccessor.bufferView];

        // Determine byte stride
        uint32_t byteStride;
        if (positionAccessor.ByteStride(positionView)) {
            byteStride = positionAccessor.ByteStride(positionView);
        } else {
            byteStride = tinygltf::GetComponentSizeInBytes(TINYGLTF_TYPE_VEC3);
        }

        auto dataOffset = positionAccessor.byteOffset + positionView.byteOffset;

        if (byteStride == 12) {
            auto buffer = reinterpret_cast<const float *>(&(model.buffers[positionView.buffer].data[dataOffset]));
            const float *position = &buffer[index * static_cast<uint32_t>(byteStride / sizeof(buffer[0]))];
            vertex.pos = glm::make_vec3(position);
        } else {
            throw std::runtime_error("glTF model contains invalid byte stride for vertex position.");
        }
    }

    void GLTFLoader::loadVertexNormal(const tinygltf::Model &model,
                                      const tinygltf::Primitive &primitive,
                                      Vertex &vertex,
                                      const uint32_t index) {
        if (primitive.attributes.find(FIELD_VERTEX_NORMAL) == primitive.attributes.end()) {
            // Skip loading joints if the model does not contain any.
            return;
        }

        const auto &normalAccessor = model.accessors[primitive.attributes.find(FIELD_VERTEX_NORMAL)->second];
        const auto &normalView = model.bufferViews[normalAccessor.bufferView];

        // Determine byte stride
        uint32_t byteStride;
        if (normalAccessor.ByteStride(normalView)) {
            byteStride = normalAccessor.ByteStride(normalView);
        } else {
            byteStride = tinygltf::GetComponentSizeInBytes(TINYGLTF_TYPE_VEC3);
        }

        auto dataOffset = normalAccessor.byteOffset + normalView.byteOffset;

        if (byteStride == 12) {
            auto buffer = reinterpret_cast<const float *>(&(model.buffers[normalView.buffer].data[dataOffset]));
            const float *normal = &buffer[index * static_cast<uint32_t>(byteStride / sizeof(buffer[0]))];
            vertex.normal = glm::make_vec3(normal);
        } else {
            throw std::runtime_error("glTF model contains invalid byte stride for vertex normal.");
        }
    }

    void GLTFLoader::loadVertexColor(const tinygltf::Model &model,
                                     const tinygltf::Primitive &primitive,
                                     Vertex &vertex,
                                     const uint32_t index) {
        if (primitive.attributes.find(FIELD_VERTEX_COLOR_0) == primitive.attributes.end()) {
            // Make the default color of a mesh white, so it's easily visible.
            vertex.color = glm::vec3(1.0f);

            return;
        }

        const auto &colorAccessor = model.accessors[primitive.attributes.find(FIELD_VERTEX_COLOR_0)->second];
        const auto &colorView = model.bufferViews[colorAccessor.bufferView];

        // Determine byte stride
        uint32_t byteStride;
        if (colorAccessor.ByteStride(colorView)) {
            byteStride = colorAccessor.ByteStride(colorView);
        } else {
            byteStride = tinygltf::GetComponentSizeInBytes(TINYGLTF_TYPE_VEC3);
        }

        auto dataOffset = colorAccessor.byteOffset + colorView.byteOffset;
        auto buffer = reinterpret_cast<const float *>(&(model.buffers[colorView.buffer].data[dataOffset]));

        switch (model.accessors[primitive.attributes.find("COLOR_0")->second].type) {
            case TINYGLTF_TYPE_VEC3: {
                const float *color = &buffer[index * 3];
                vertex.color = glm::vec3(color[0], color[1], color[2]);

                break;
            }
            case TINYGLTF_TYPE_VEC4: {
                const float *color = &buffer[index * 4];
                vertex.color = glm::vec3(color[0], color[1], color[2]);

                break;
            }
            default: {
                throw std::runtime_error("glTF model contains invalid byte stride for vertex normal.");
            }
        }

        if (byteStride == 12) {
            auto colorBuffer = reinterpret_cast<const float *>(&(model.buffers[colorView.buffer].data[dataOffset]));
            const float *normal = &colorBuffer[index * static_cast<uint32_t>(byteStride / sizeof(colorBuffer[0]))];
            vertex.normal = glm::make_vec3(normal);
        } else {
            throw std::runtime_error("glTF model contains invalid byte stride for vertex normal.");
        }
    }


    void GLTFLoader::loadVertexUV0(const tinygltf::Model &model,
                                   const tinygltf::Primitive &primitive,
                                   Vertex &vertex,
                                   const uint32_t index) {
        if (primitive.attributes.find(FIELD_VERTEX_TEXCOORD_0) == primitive.attributes.end()) {
            // Skip loading joints if the model does not contain any.
            return;
        }

        const auto &normalAccessor = model.accessors[primitive.attributes.find(FIELD_VERTEX_TEXCOORD_0)->second];
        const auto &normalView = model.bufferViews[normalAccessor.bufferView];

        // Determine byte stride
        uint32_t byteStride;
        if (normalAccessor.ByteStride(normalView)) {
            byteStride = normalAccessor.ByteStride(normalView);
        } else {
            byteStride = tinygltf::GetComponentSizeInBytes(TINYGLTF_TYPE_VEC2);
        }

        auto dataOffset = normalAccessor.byteOffset + normalView.byteOffset;

        if (byteStride == 8) {
            auto buffer = reinterpret_cast<const float *>(&(model.buffers[normalView.buffer].data[dataOffset]));
            const float *uv0 = &buffer[index * static_cast<uint32_t>(byteStride / sizeof(buffer[0]))];
            vertex.UV0 = glm::make_vec2(uv0);
        } else {
            throw std::runtime_error("glTF model contains invalid byte stride for vertex UV0 coordinates.");
        }
    }

    void GLTFLoader::loadVertexUV1(const tinygltf::Model &model,
                                   const tinygltf::Primitive &primitive,
                                   Vertex &vertex,
                                   const uint32_t index) {
        if (primitive.attributes.find(FIELD_VERTEX_TEXCOORD_1) == primitive.attributes.end()) {
            // Skip loading joints if the model does not contain any.
            return;
        }

        const auto &normalAccessor = model.accessors[primitive.attributes.find(FIELD_VERTEX_TEXCOORD_1)->second];
        const auto &normalView = model.bufferViews[normalAccessor.bufferView];

        // Determine byte stride
        uint32_t byteStride;
        if (normalAccessor.ByteStride(normalView)) {
            byteStride = normalAccessor.ByteStride(normalView);
        } else {
            byteStride = tinygltf::GetComponentSizeInBytes(TINYGLTF_TYPE_VEC2);
        }

        auto dataOffset = normalAccessor.byteOffset + normalView.byteOffset;

        if (byteStride == 8) {
            auto buffer = reinterpret_cast<const float *>(&(model.buffers[normalView.buffer].data[dataOffset]));
            const float *uv1 = &buffer[index * static_cast<uint32_t>(byteStride / sizeof(buffer[0]))];
            vertex.UV1 = glm::make_vec2(uv1);
        } else {
            throw std::runtime_error("glTF model contains invalid byte stride for vertex UV1 coordinates.");
        }
    }

    void GLTFLoader::loadVertexJoints(const tinygltf::Model &model,
                                      const tinygltf::Primitive &primitive,
                                      Vertex &vertex,
                                      const uint32_t index) {
        if (primitive.attributes.find(FIELD_VERTEX_JOINTS_0) == primitive.attributes.end()) {
            // Skip loading joints if the model does not contain any.
            return;
        }

        const auto &jointAccessor = model.accessors[primitive.attributes.find(FIELD_VERTEX_JOINTS_0)->second];
        const auto &jointView = model.bufferViews[jointAccessor.bufferView];

        // Determine byte stride
        uint32_t byteStride;
        if (jointAccessor.ByteStride(jointView)) {
            byteStride = jointAccessor.ByteStride(jointView);
        } else {
            byteStride = tinygltf::GetComponentSizeInBytes(TINYGLTF_TYPE_VEC4);
        }

        auto dataOffset = jointAccessor.byteOffset + jointView.byteOffset;

        switch (byteStride) {
            case 4: {
                auto buffer = reinterpret_cast<const uint8_t *>(&(model.buffers[jointView.buffer].data[dataOffset]));
                const uint8_t *joint = &buffer[index * static_cast<uint32_t>(byteStride / sizeof(buffer[0]))];
                vertex.joint = glm::ivec4(joint[0], joint[1], joint[2], joint[3]);
                break;
            }
            case 8: {
                auto buffer = reinterpret_cast<const uint16_t *>(&(model.buffers[jointView.buffer].data[dataOffset]));
                const uint16_t *joint = &buffer[index * static_cast<uint32_t>(byteStride / sizeof(buffer[0]))];
                vertex.joint = glm::ivec4(joint[0], joint[1], joint[2], joint[3]);
                break;
            }
            default: {
                throw std::runtime_error("glTF model contains invalid byte stride for vertex joints.");
            }
        }
    }

    void GLTFLoader::loadVertexWeights(const tinygltf::Model &model,
                                       const tinygltf::Primitive &primitive,
                                       Vertex &vertex,
                                       const uint32_t index) {
        if (primitive.attributes.find(FIELD_VERTEX_WEIGHTS_0) == primitive.attributes.end()) {
            // Skip loading weights if the model does not contain any.
            return;
        }

        const auto &weightAccessor = model.accessors[primitive.attributes.find(FIELD_VERTEX_WEIGHTS_0)->second];
        const auto &weightView = model.bufferViews[weightAccessor.bufferView];

        // Determine byte stride
        uint32_t byteStride;
        if (weightAccessor.ByteStride(weightView)) {
            byteStride = weightAccessor.ByteStride(weightView);
        } else {
            byteStride = tinygltf::GetComponentSizeInBytes(TINYGLTF_TYPE_VEC4);
        }

        auto dataOffset = weightAccessor.byteOffset + weightView.byteOffset;

        if (byteStride == 16) {
            auto buffer = reinterpret_cast<const float *>(&(model.buffers[weightView.buffer].data[dataOffset]));
            const float *weight = &buffer[index * static_cast<uint32_t>(byteStride / sizeof(buffer[0]))];
            vertex.weight = glm::make_vec4(weight);
        } else {
            throw std::runtime_error("glTF model contains invalid byte stride for vertex weights.");
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
                memcpy(buffer, &indexBuffer.data[indexAccessor.byteOffset + indexBufferView.byteOffset],
                       sizeof(uint16_t) * indexAccessor.count);
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

    std::map<uint32_t, std::vector<gltf::Primitive *>>
    GLTFLoader::initializePrimitiveLookupTable(std::vector<gltf::Node *> &nodes) {
        std::map<uint32_t, std::vector<gltf::Primitive *>> primitiveLookup;

        for (auto &node : nodes) {
            auto allPrimitive = getAllPrimitive(node);
            primitiveLookup[node->nodeIndex] = allPrimitive;
        }

        return primitiveLookup;
    }
}
