//  Heavily inspired on: https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanglTFmodel->cpp
//  PvkGLTFLoader.cpp
//  PVK
//
//  Created by Christian aan de Wiel on 02/01/2021.
//

#include <numeric>
#include <utility>
#include "GLTFLoader.hpp"

namespace pvk {
    inline auto endsWith(std::string const &value, std::string const &ending) -> bool {
        if (ending.size() > value.size()) {
            return false;
        }

        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    }

    auto GLTFLoader::loadObject(vk::Queue &graphicsQueue,
                                const std::string &filePath)
    -> std::unique_ptr<gltf::Object> {
        tinygltf::TinyGLTF loader;
        auto model = std::make_shared<tinygltf::Model>();
        std::string error;
        std::string warning;

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        bool isModelLoaded = false;

        auto t1 = std::chrono::high_resolution_clock::now();

        if (endsWith(filePath, EXTENSION_GLB)) {
            isModelLoaded = loader.LoadBinaryFromFile(model.get(), &error, &warning, filePath);
        } else {
            isModelLoaded = loader.LoadASCIIFromFile(model.get(), &error, &warning, filePath);
        }

        auto t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        std::cout << "[TINYGLTF] Loading model took " << duration << "ms" << std::endl;

        if (!isModelLoaded) {
            throw std::runtime_error("Could not load glTF model");
        }
        t1 = std::chrono::high_resolution_clock::now();

        auto object = std::make_unique<gltf::Object>();

        auto primitiveLookup = GLTFLoader::loadPrimitives(model, graphicsQueue, *object);

        object->nodes = GLTFLoader::loadNodes(model, primitiveLookup, graphicsQueue, *object);
        object->nodeLookup = GLTFLoader::initializeNodeLookupTable(object->nodes);
        object->primitiveLookup = GLTFLoader::initializePrimitiveLookupTable(object->nodes);
        object->animations = GLTFLoader::loadAnimations(model, object->nodeLookup);
        GLTFLoader::loadMaterials(model, graphicsQueue, *object);

//        buffer::create<Vertex, vk::BufferUsageFlagBits::eVertexBuffer>(graphicsQueue,
//                                                                       object->vertexBuffer,
//                                                                       object->vertexBufferMemory,
//                                                                       object->vertices);
        buffer::vertex::create(graphicsQueue,
                               object->vertexBuffer,
                               object->vertexBufferMemory,
                               object->vertices);

        if (!object->indices.empty()) {
            buffer::index::create(graphicsQueue,
                                  object->indexBuffer,
                                  object->indexBufferMemory,
                                  object->indices);
        }

        t2 = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        std::cout << "[PVK] Loading model took " << duration << "ms" << std::endl;

        return object;
    }

    auto GLTFLoader::createVertexBuffer(vk::Queue &graphicsQueue,
                                        const std::unique_ptr<gltf::Object> &object
    ) -> std::future<void> {
        return std::async(std::launch::async, [&, &object = *object] {
            buffer::vertex::create(graphicsQueue,
                                   object.vertexBuffer,
                                   object.vertexBufferMemory,
                                   object.vertices);
        });
    }

    auto GLTFLoader::createIndexBuffer(vk::Queue &graphicsQueue,
                                       const std::unique_ptr<gltf::Object> &object
    ) -> std::future<void> {
        return std::async(std::launch::async, [&, &object = *object] {
            if (!object.indices.empty()) {
                buffer::index::create(graphicsQueue,
                                      object.indexBuffer,
                                      object.indexBufferMemory,
                                      object.indices);
            }
        });
    }

    auto GLTFLoader::loadNodes(const std::shared_ptr<tinygltf::Model> &model,
                               std::vector<std::vector<std::unique_ptr<gltf::Primitive>>> &primitiveLookup,
                               vk::Queue &graphicsQueue,
                               gltf::Object &object)
    -> std::vector<std::shared_ptr<gltf::Node>> {
        const auto &scene = model->scenes[model->defaultScene];

        std::vector<std::shared_ptr<gltf::Node>> nodes{};

        for (const auto &nodeIndex : scene.nodes) {
            auto _node = GLTFLoader::loadNode(model, primitiveLookup, nodeIndex, graphicsQueue, object);

            if (_node->children.empty() && _node->mesh == nullptr) {
                // Don't add this, most likely light or camera.
            } else {
                nodes.push_back(_node);
            }
        }

        return nodes;
    }

    auto GLTFLoader::loadNode(const std::shared_ptr<tinygltf::Model> &model,
                              std::vector<std::vector<std::unique_ptr<gltf::Primitive>>> &primitiveLookup,
                              uint32_t nodeIndex,
                              vk::Queue &graphicsQueue,
                              gltf::Object &object,
                              std::shared_ptr<gltf::Node> parent)
    -> std::shared_ptr<gltf::Node> {
        auto &node = model->nodes[nodeIndex];

        auto resultNode = std::make_shared<gltf::Node>();
        resultNode->parent = std::move(parent);
        resultNode->skinIndex = node.skin;
        resultNode->nodeIndex = nodeIndex;
        resultNode->matrix = glm::mat4(1.0F);
        resultNode->name = node.name;

        // First load all children recursively
        resultNode->children.reserve(node.children.size());

        for (auto &child : node.children) {
            resultNode->children.emplace_back(
                    loadNode(model, primitiveLookup, child, graphicsQueue, object, resultNode));
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
            resultNode->matrix = glm::mat4(1.0F);
        }

        // Filter out all lights and cameras
        if (node.mesh == -1) {
            return resultNode;
        }

        auto &mesh = model->meshes[node.mesh];

        resultNode->mesh = std::make_unique<Mesh>();
        resultNode->primitives.reserve(mesh.primitives.size());

        for (auto &&primitive : primitiveLookup[node.mesh]) {
            resultNode->primitives.emplace_back(std::move(primitive));
        }

        if (node.skin > -1) {
            auto &skin = model->skins[node.skin];
            auto _skin = std::make_shared<gltf::Skin>();

            _skin->skinIndex = node.skin;
            _skin->jointsIndices.reserve(skin.joints.size());

            for (auto &joint : skin.joints) {
                _skin->jointsIndices.emplace_back(joint);
            }

            // Check whether there are inverse bind matrices present
            if (skin.inverseBindMatrices > -1) {
                const auto &accessor = model->accessors[skin.inverseBindMatrices];
                const auto &bufferView = model->bufferViews[accessor.bufferView];
                const auto &buffer = model->buffers[bufferView.buffer];

                _skin->inverseBindMatrices.resize(accessor.count);
                memcpy(_skin->inverseBindMatrices.data(),
                       &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                       accessor.count * sizeof(glm::mat4));
            }

            object.skinLookup[node.skin] = _skin;
        }

        return resultNode;
    }

    template<typename T, uint8_t U, size_t size>
    auto loadBuffer(const std::shared_ptr<tinygltf::Model> &model,
                    const tinygltf::Primitive &primitive,
                    const std::string &field,
                    Vertex &vertex,
                    uint32_t index) -> T {
        const auto &accessor = model->accessors[primitive.attributes.find(field)->second];
        const auto &bufferView = model->bufferViews[accessor.bufferView];

        // Determine byte stride
        uint32_t byteStride = 0;
        if (accessor.ByteStride(bufferView) > 1) {
            byteStride = accessor.ByteStride(bufferView);
        } else {
            byteStride = tinygltf::GetComponentSizeInBytes(U);
        }

        auto dataOffset = accessor.byteOffset + bufferView.byteOffset;

        if (byteStride == size) {
            const auto *buffer = reinterpret_cast<T>(&(model->buffers[bufferView.buffer].data[dataOffset]));
            return &buffer[index * static_cast<uint32_t>(byteStride / sizeof(buffer[0]))];
        }

        throw std::runtime_error("glTF model contains invalid byte stride.");
    }

    void GLTFLoader::loadVertexPosition(const std::shared_ptr<tinygltf::Model> &model,
                                        const tinygltf::Primitive &primitive,
                                        Vertex &vertex,
                                        const uint32_t index) {
        vertex.pos = glm::make_vec3(loadBuffer<const float *, TINYGLTF_TYPE_VEC3, sizeof(glm::vec3)>
                                            (model, primitive, FIELD_VERTEX_POSITION, vertex, index)
        );
    }

    void GLTFLoader::loadVertexNormal(const std::shared_ptr<tinygltf::Model> &model,
                                      const tinygltf::Primitive &primitive,
                                      Vertex &vertex,
                                      const uint32_t index) {
        if (primitive.attributes.find(FIELD_VERTEX_NORMAL) == primitive.attributes.end()) {
            // Skip loading normals if the model does not contain any.
            return;
        }

        vertex.normal = glm::make_vec3(loadBuffer<const float *, TINYGLTF_TYPE_VEC3, sizeof(glm::vec3)>
                                               (model, primitive, FIELD_VERTEX_NORMAL, vertex, index));
    }

    void GLTFLoader::loadVertexColor(const std::shared_ptr<tinygltf::Model> &model,
                                     const tinygltf::Primitive &primitive,
                                     Vertex &vertex,
                                     const uint32_t index) {
        if (primitive.attributes.find(FIELD_VERTEX_COLOR_0) == primitive.attributes.end()) {
            // Make the default color of a mesh white, so it's easily visible.
            vertex.color = glm::vec3(1.0F);

            return;
        }

        switch (model->accessors[primitive.attributes.find("COLOR_0")->second].type) {
            case TINYGLTF_TYPE_VEC3: {
                auto color = std::span<const float>(loadBuffer<const float *, TINYGLTF_TYPE_VEC3, sizeof(glm::vec4)>
                                                            (model, primitive, FIELD_VERTEX_COLOR_0, vertex, index), 3);
                vertex.color = glm::vec3(color[0], color[1], color[2]);

                break;
            }
            case TINYGLTF_TYPE_VEC4: {
                auto color = std::span<const float>(loadBuffer<const float *, TINYGLTF_TYPE_VEC4, sizeof(glm::vec4)>
                                                            (model, primitive, FIELD_VERTEX_COLOR_0, vertex, index), 3);
                vertex.color = glm::vec3(color[0], color[1], color[2]);

                break;
            }
            default: {
                throw std::runtime_error("Invalid vertex color type.");
            }
        }
    }

    void GLTFLoader::loadVertexUV0(const std::shared_ptr<tinygltf::Model> &model,
                                   const tinygltf::Primitive &primitive,
                                   Vertex &vertex,
                                   const uint32_t index) {
        if (primitive.attributes.find(FIELD_VERTEX_TEXCOORD_0) == primitive.attributes.end()) {
            // Skip loading joints if the model does not contain any.
            return;
        }

        vertex.UV0 = glm::make_vec2(loadBuffer<const float *, TINYGLTF_TYPE_VEC2, sizeof(glm::vec2)>
                                            (model, primitive, FIELD_VERTEX_TEXCOORD_0, vertex, index));
    }

    void GLTFLoader::loadVertexUV1(const std::shared_ptr<tinygltf::Model> &model,
                                   const tinygltf::Primitive &primitive,
                                   Vertex &vertex,
                                   const uint32_t index) {
        if (primitive.attributes.find(FIELD_VERTEX_TEXCOORD_1) == primitive.attributes.end()) {
            // Skip loading joints if the model does not contain any.
            return;
        }

        vertex.UV1 = glm::make_vec2(loadBuffer<const float *, TINYGLTF_TYPE_VEC2, sizeof(glm::vec2)>
                                            (model, primitive, FIELD_VERTEX_TEXCOORD_1, vertex, index));
    }

    void GLTFLoader::loadVertexJoints(const std::shared_ptr<tinygltf::Model> &model,
                                      const tinygltf::Primitive &primitive,
                                      Vertex &vertex,
                                      const uint32_t index) {
        if (primitive.attributes.find(FIELD_VERTEX_JOINTS_0) == primitive.attributes.end()) {
            // Skip loading joints if the model does not contain any.
            return;
        }

        const auto &jointAccessor = model->accessors[primitive.attributes.find(FIELD_VERTEX_JOINTS_0)->second];
        const auto &jointView = model->bufferViews[jointAccessor.bufferView];

        // Determine byte stride
        uint32_t byteStride = 0;
        if (jointAccessor.ByteStride(jointView) > -1) {
            byteStride = jointAccessor.ByteStride(jointView);
        } else {
            byteStride = tinygltf::GetComponentSizeInBytes(TINYGLTF_TYPE_VEC4);
        }

        switch (byteStride) {
            case 4: {
                vertex.joint = glm::make_vec4(loadBuffer<const uint8_t *, TINYGLTF_TYPE_VEC4, sizeof(uint8_t) * 4>
                                                      (model, primitive, FIELD_VERTEX_JOINTS_0, vertex, index));
                break;
            }
            case 8: {
                vertex.joint = glm::make_vec4(loadBuffer<const uint16_t *, TINYGLTF_TYPE_VEC4, sizeof(uint16_t) * 4>
                                                      (model, primitive, FIELD_VERTEX_JOINTS_0, vertex, index));
                break;
            }
            default: {
                throw std::runtime_error("glTF model contains invalid byte stride for vertex joints.");
            }
        }
    }

    void GLTFLoader::loadVertexWeights(const std::shared_ptr<tinygltf::Model> &model,
                                       const tinygltf::Primitive &primitive,
                                       Vertex &vertex,
                                       const uint32_t index) {
        if (primitive.attributes.find(FIELD_VERTEX_WEIGHTS_0) == primitive.attributes.end()) {
            // Skip loading weights if the model does not contain any.
            return;
        }

        vertex.weight = glm::make_vec4(loadBuffer<const float *, TINYGLTF_TYPE_VEC4, sizeof(glm::vec4)>
                                               (model, primitive, FIELD_VERTEX_WEIGHTS_0, vertex, index));
    }

    void GLTFLoader::loadMaterials(const std::shared_ptr<tinygltf::Model> &model,
                                   vk::Queue &graphicsQueue,
                                   gltf::Object &object) {
        auto materials = std::vector<gltf::Material *>(model->materials.size());

        auto loadTexture = [&](const int8_t textureIndex) {
            auto _texture = std::make_unique<Texture>();

            if (textureIndex > -1) {
                auto &baseColorTexture = model->textures[textureIndex];
                auto &baseColorTextureImage = model->images[baseColorTexture.source];
                buffer::texture::create(graphicsQueue, baseColorTextureImage, *_texture);
            } else {
                buffer::texture::createEmpty(graphicsQueue, *_texture);
            }

            return _texture;
        };

        for (auto i = 0; i < static_cast<int>(model->materials.size()); i++) {
            auto &material = model->materials[i];
            auto _material = new gltf::Material();

            _material->baseColorTexture = loadTexture(material.pbrMetallicRoughness.baseColorTexture.index);
            _material->metallicRoughnessTexture = loadTexture(
                    material.pbrMetallicRoughness.metallicRoughnessTexture.index);
            _material->occlusionTexture = loadTexture(material.occlusionTexture.index);

            materials[i] = _material;
        }

        object.materials = std::move(materials);
    }

    auto GLTFLoader::loadAnimations(const std::shared_ptr<tinygltf::Model> &model,
                                    const std::map<uint32_t, std::shared_ptr<gltf::Node>> &nodeLookup
    ) -> std::vector<std::unique_ptr<gltf::Animation>> {
        auto loadAnimationInputs = [&model](tinygltf::AnimationSampler &sampler, gltf::Sampler &_sampler) {
            auto &accessor = model->accessors[sampler.input];
            auto &bufferView = model->bufferViews[accessor.bufferView];
            auto &buffer = model->buffers[bufferView.buffer];

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
            auto &accessor = model->accessors[sampler.output];
            auto &bufferView = model->bufferViews[accessor.bufferView];
            auto &buffer = model->buffers[bufferView.buffer];

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
            _channel.node = nodeLookup.at(channel.target_node);
        };

        std::vector<std::unique_ptr<gltf::Animation>> animations;
        animations.reserve(model->animations.size());

        for (auto &animation : model->animations) {
            auto _animation = std::make_unique<gltf::Animation>();
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

            _animation->currentTime = 0.0F;
            _animation->startTime = 0.0F;
            _animation->endTime = -1.0F;

            for (auto &sampler : _animation->samplers) {
                for (auto &input : sampler.inputs) {
                    if (_animation->endTime == -1.0F) {
                        _animation->endTime = input;
                    } else if (input > _animation->endTime) {
                        _animation->endTime = input;
                    }
                }
            }

            animations.emplace_back(std::move(_animation));
        }

        return animations;
    }

    inline auto getAllNode(const std::shared_ptr<gltf::Node> &node) -> std::vector<std::shared_ptr<gltf::Node>> {
        std::vector<std::shared_ptr<gltf::Node>> allNodes;
        allNodes.emplace_back(node);

        for (auto &child : node->children) {
            auto allChildrenNodes = getAllNode(child);
            allNodes.reserve(allNodes.size() + allChildrenNodes.size());
            allNodes.insert(allNodes.end(), allChildrenNodes.begin(), allChildrenNodes.end());
        }

        return allNodes;
    }

    auto GLTFLoader::initializeNodeLookupTable(
            std::vector<std::shared_ptr<gltf::Node>> &nodes
    ) -> std::map<uint32_t, std::shared_ptr<gltf::Node>> {
        std::map<uint32_t, std::shared_ptr<gltf::Node>> nodeLookup{};

        for (auto &rootNode : nodes) {
            auto allNodes = getAllNode(rootNode);

            for (auto &node : allNodes) {
                nodeLookup[node->nodeIndex] = node;
            }
        }

        return nodeLookup;
    }

    inline auto getAllPrimitive(const std::shared_ptr<gltf::Node> &node) -> std::vector<gltf::Primitive *> {
        std::vector<gltf::Primitive *> allPrimitives;

        for (auto &primitive : node->primitives) {
            allPrimitives.emplace_back(primitive.get());
        }

        for (auto &child : node->children) {
            auto allPrimitiveChildren = getAllPrimitive(child);
            std::move(allPrimitiveChildren.begin(), allPrimitiveChildren.end(), std::back_inserter(allPrimitives));
        }

        return allPrimitives;
    }

    auto GLTFLoader::initializePrimitiveLookupTable(
            std::vector<std::shared_ptr<gltf::Node>> &nodes) -> std::map<uint32_t, std::vector<gltf::Primitive *>> {
        std::map<uint32_t, std::vector<gltf::Primitive *>> primitiveLookup;

        for (auto &node : nodes) {
            primitiveLookup[node->nodeIndex] = getAllPrimitive(node);
        }

        return primitiveLookup;
    }

    template<typename T>
    auto flatten(std::vector<std::future<std::vector<T>>> &&t) -> std::future<std::vector<T>> {
        return std::async(std::launch::async, [t = std::move(t)]() mutable {
            std::vector<T> result;
            for (auto &fut : t) {
                auto allElement = fut.get();
                std::move(allElement.begin(), allElement.end(), std::back_inserter(result));
            }
            return result;
        });
    }

    auto GLTFLoader::loadPrimitives(const std::shared_ptr<tinygltf::Model> &model,
                                    vk::Queue &graphicsQueue,
                                    gltf::Object &object)
    -> std::vector<std::vector<std::unique_ptr<gltf::Primitive>>> {
        std::vector<tinygltf::Primitive *> primitives;
        std::vector<std::vector<std::unique_ptr<gltf::Primitive>>> primitiveLookup;
        std::vector<std::pair<size_t, size_t>> vertexAccessorIndexBatches;
        std::vector<uint32_t> numberOfVertexPerPrimitive;
        std::vector<uint32_t> numberOfIndexPerPrimitive;
        std::vector<uint32_t> vertexOffsets;
        std::vector<uint32_t> indexOffsets;

        uint32_t currentVertexOffset = 0;
        uint32_t currentIndexOffset = 0;

        primitiveLookup.reserve(model->meshes.size());

        for (auto &mesh : model->meshes) {
            std::vector<std::unique_ptr<gltf::Primitive>> meshPrimitives;
            meshPrimitives.reserve(mesh.primitives.size());

            for (auto &primitive : mesh.primitives) {
                const auto currentPrimitiveIndex = primitives.size();
                const auto vertexCount = model->accessors[primitive.attributes.find(
                        FIELD_VERTEX_POSITION)->second].count;
                const auto indexCount = model->accessors[primitive.indices].count;

                vertexOffsets.emplace_back(currentVertexOffset);
                indexOffsets.emplace_back(currentIndexOffset);

                for (size_t i = 0; i < vertexCount; i++) {
                    vertexAccessorIndexBatches.emplace_back(std::make_pair(i, currentPrimitiveIndex));
                }

                primitives.emplace_back(&primitive);
                numberOfVertexPerPrimitive.emplace_back(vertexCount);
                numberOfIndexPerPrimitive.emplace_back(indexCount);

                auto _primitive = std::make_unique<gltf::Primitive>();
                _primitive->startVertex = currentVertexOffset;
                _primitive->startIndex = currentIndexOffset;
                _primitive->vertexCount = vertexCount;
                _primitive->indexCount = indexCount;
                meshPrimitives.emplace_back(std::move(_primitive));

                currentVertexOffset += vertexCount;
                currentIndexOffset += indexCount;
            }

            primitiveLookup.emplace_back(std::move(meshPrimitives));
        }

        std::vector<std::future<std::vector<Vertex>>> primitiveVertices;
        std::vector<std::future<std::vector<uint32_t>>> primitiveIndices;

        for (size_t i = 0; i < ceil(static_cast<float>(vertexAccessorIndexBatches.size()) /
                                    static_cast<float>(VERTEX_BATCH_SIZE)); i++) {
            primitiveVertices.emplace_back(loadVerticesByPrimitive(model, primitives, vertexAccessorIndexBatches,
                                                                   i * VERTEX_BATCH_SIZE));
        }

        for (size_t i = 0; i < primitives.size(); i++) {
            primitiveIndices.emplace_back(loadIndicesByPrimitive(model, primitives[i], vertexOffsets[i]));
        }

        object.vertices = flatten(std::move(primitiveVertices)).get();
        object.indices = flatten(std::move(primitiveIndices)).get();

        return primitiveLookup;
    }

    auto GLTFLoader::loadVerticesByPrimitive(std::shared_ptr<tinygltf::Model> model,
                                             const std::vector<tinygltf::Primitive *> &primitives,
                                             const std::vector<std::pair<size_t, size_t>> &primitiveIndexPairs,
                                             uint32_t startingIndex) -> std::future<std::vector<Vertex>> {
        return std::async(std::launch::async, [&, model = std::move(model), startingIndex] {
            std::vector<Vertex> vertices;
            vertices.reserve(VERTEX_BATCH_SIZE);
            const auto indexEnd = std::min(primitiveIndexPairs.size(),
                                           startingIndex + static_cast<size_t>(VERTEX_BATCH_SIZE));

            for (size_t i = startingIndex; i < indexEnd; i++) {
                Vertex vertex{};
                const auto &attribute = primitiveIndexPairs[i];
                const auto &primitive = primitives[attribute.second];

                GLTFLoader::loadVertexPosition(model, *primitive, vertex, attribute.first);
                GLTFLoader::loadVertexNormal(model, *primitive, vertex, attribute.first);
                GLTFLoader::loadVertexUV0(model, *primitive, vertex, attribute.first);
                GLTFLoader::loadVertexUV1(model, *primitive, vertex, attribute.first);
                GLTFLoader::loadVertexJoints(model, *primitive, vertex, attribute.first);
                GLTFLoader::loadVertexWeights(model, *primitive, vertex, attribute.first);
                GLTFLoader::loadVertexColor(model, *primitive, vertex, attribute.first);

                vertices.emplace_back(vertex);
            }

            return vertices;
        });
    }


    auto GLTFLoader::loadIndicesByPrimitive(const std::shared_ptr<tinygltf::Model> &model,
                                            const tinygltf::Primitive *primitive,
                                            uint32_t vertexStart) -> std::future<std::vector<uint32_t>> {
        return std::async(std::launch::async, [model = std::move(model), primitive, vertexStart] {
            std::vector<uint32_t> indices;

            if (primitive->indices == -1) {
                // Model has no indices.
                return indices;
            }

            const tinygltf::Accessor &indexAccessor = model->accessors[primitive->indices];
            const tinygltf::BufferView &indexBufferView = model->bufferViews[indexAccessor.bufferView];
            const tinygltf::Buffer &indexBuffer = model->buffers[indexBufferView.buffer];
            indices.reserve(indexAccessor.count);

            switch (indexAccessor.componentType) {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                    auto *buffer = new uint32_t[indexAccessor.count];
                    memcpy(buffer, &indexBuffer.data[indexAccessor.byteOffset + indexBufferView.byteOffset],
                           sizeof(uint32_t) * indexAccessor.count);
                    for (size_t index = 0; index < indexAccessor.count; index++) {
                        indices.emplace_back(buffer[index] + vertexStart);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                    auto *buffer = new uint16_t[indexAccessor.count];
                    memcpy(buffer, &indexBuffer.data[indexAccessor.byteOffset + indexBufferView.byteOffset],
                           sizeof(uint16_t) * indexAccessor.count);
                    for (size_t index = 0; index < indexAccessor.count; index++) {
                        indices.emplace_back(buffer[index] + vertexStart);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                    auto *buffer = new uint8_t[indexAccessor.count];
                    memcpy(buffer, &indexBuffer.data[indexAccessor.byteOffset + indexBufferView.byteOffset],
                           sizeof(uint8_t) * indexAccessor.count);
                    for (size_t index = 0; index < indexAccessor.count; index++) {
                        indices.emplace_back(buffer[index] + vertexStart);
                    }
                    break;
                }
                default: {
                    throw std::runtime_error("Unsupported glTF index component type");
                }
            }

            return indices;
        });
    }
}
