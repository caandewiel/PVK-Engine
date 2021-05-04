//  Heavily inspired on:
//  https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanglTFmodel->cpp
//  PvkGLTFLoader.cpp
//  PVK
//
//  Created by Christian aan de Wiel on 02/01/2021.
//
#include "GLTFLoader.hpp"
#include "loader/GLTFLoaderNode.hpp"
#include "loader/GLTFLoaderVertex.hpp"
#include "loader/GLTFLoaderAnimation.hpp"
#include "loader/GLTFLoaderMaterial.hpp"

#include <numeric>
#include <utility>
#include <span>

namespace {
    bool endsWith(std::string const &value, std::string const &ending) {
        if (ending.size() > value.size()) {
            return false;
        }

        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    }

    inline std::vector<std::shared_ptr<pvk::gltf::Node>>
    getAllNode(const std::shared_ptr<pvk::gltf::Node> &node) {
        std::vector<std::shared_ptr<pvk::gltf::Node>> allNodes;
        allNodes.emplace_back(node);

        for (auto &child : node->children) {
            auto allChildrenNodes = getAllNode(child);
            allNodes.insert(allNodes.end(), allChildrenNodes.begin(), allChildrenNodes.end());
        }

        return allNodes;
    }

    boost::container::flat_map<uint32_t, std::shared_ptr<pvk::gltf::Node>>
    initializeNodeLookupTable(std::vector<std::shared_ptr<pvk::gltf::Node>> &nodes) {
        boost::container::flat_map<uint32_t, std::shared_ptr<pvk::gltf::Node>> nodeLookup{};

        for (auto &rootNode : nodes) {
            auto allNodes = getAllNode(rootNode);

            for (auto &node : allNodes) {
                nodeLookup[node->nodeIndex] = node;
            }
        }

        return nodeLookup;
    }

    std::vector<std::unique_ptr<pvk::gltf::Animation>> loadAnimations(
            const tinygltf::Model &model,
            const boost::container::flat_map<uint32_t, std::shared_ptr<pvk::gltf::Node>> &nodeLookup
    ) {
        std::vector<std::unique_ptr<pvk::gltf::Animation>> animations;
        animations.reserve(model.animations.size());

        for (const auto &animation : model.animations) {
            animations.emplace_back(pvk::gltf::loader::animation::getAnimation(model, animation, nodeLookup));
        }

        return animations;
    }
}  // namespace

namespace pvk {
    std::unique_ptr<gltf::Object> GLTFLoader::loadObject(vk::Queue &graphicsQueue, const std::string &filePath) {
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

        object->nodes = pvk::gltf::loader::node::loadNodes(model, primitiveLookup, graphicsQueue, *object);
        object->setNodeLookup(initializeNodeLookupTable(object->nodes));
        object->primitiveLookup = GLTFLoader::initializePrimitiveLookupTable(object->nodes);
        object->animations = loadAnimations(*model, object->getNodes());

        buffer::vertex::create(graphicsQueue, object->vertexBuffer, object->vertexBufferMemory, object->vertices);

        if (!object->indices.empty()) {
            buffer::index::create(graphicsQueue, object->indexBuffer, object->indexBufferMemory, object->indices);
        }

        t2 = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        std::cout << "[PVK] Loading model took " << duration << "ms" << std::endl;

        return object;
    }

    inline std::vector<std::shared_ptr<gltf::Primitive>> getAllPrimitive(const std::shared_ptr<gltf::Node> &node) {
        std::vector<std::shared_ptr<gltf::Primitive>> allPrimitives;

        for (auto &primitive : node->primitives) {
            allPrimitives.emplace_back(primitive);
        }

        for (auto &child : node->children) {
            auto allPrimitiveChildren = getAllPrimitive(child);
            std::move(allPrimitiveChildren.begin(), allPrimitiveChildren.end(), std::back_inserter(allPrimitives));
        }

        return allPrimitives;
    }

    std::map<uint32_t, std::vector<std::weak_ptr<gltf::Primitive>>>
    GLTFLoader::initializePrimitiveLookupTable(std::vector<std::shared_ptr<gltf::Node>> &nodes) {
        std::map<uint32_t, std::vector<std::weak_ptr<gltf::Primitive>>> primitiveLookup;

        for (const auto &node : nodes) {
            primitiveLookup[node->nodeIndex] = {};

            for (const auto &primitive : getAllPrimitive(node)) {
                primitiveLookup[node->nodeIndex].emplace_back(primitive);
            }
        }

        return primitiveLookup;
    }

    template<typename T>
    std::future<std::vector<T>> flatten(std::vector<std::future<std::vector<T>>> &&t) {
        return std::async(std::launch::async, [t = std::move(t)]() mutable {
            std::vector<T> result;
            for (auto &fut : t) {
                auto allElement = fut.get();
                std::move(allElement.begin(), allElement.end(), std::back_inserter(result));
            }
            return result;
        });
    }

    std::vector<std::vector<std::shared_ptr<gltf::Primitive>>> GLTFLoader::loadPrimitives(
            const std::shared_ptr<tinygltf::Model> &model,
            const vk::Queue &graphicsQueue,
            gltf::Object &object
    ) {
        std::vector<tinygltf::Primitive *> primitives;
        std::vector<std::vector<std::shared_ptr<gltf::Primitive>>> primitiveLookup;
        std::vector<std::pair<size_t, size_t>> vertexAccessorIndexBatches;
        std::vector<uint32_t> numberOfVertexPerPrimitive;
        std::vector<uint32_t> numberOfIndexPerPrimitive;
        std::vector<uint32_t> vertexOffsets;
        std::vector<uint32_t> indexOffsets;

        uint32_t currentVertexOffset = 0;
        uint32_t currentIndexOffset = 0;

        primitiveLookup.reserve(model->meshes.size());

        for (auto &mesh : model->meshes) {
            std::cout << "Processing mesh" << std::endl;
            std::vector<std::shared_ptr<gltf::Primitive>> meshPrimitives;
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

                auto _primitive = std::make_shared<gltf::Primitive>(
                        currentVertexOffset,
                        currentIndexOffset,
                        vertexCount,
                        indexCount
                );
                _primitive->material = gltf::loader::material::getMaterial(*model, primitive.material);
                meshPrimitives.emplace_back(std::move(_primitive));

                currentVertexOffset += vertexCount;
                currentIndexOffset += indexCount;
            }

            primitiveLookup.emplace_back(std::move(meshPrimitives));
        }

        std::vector<std::future<std::vector<Vertex>>> primitiveVertices;
        std::vector<std::future<std::vector<uint32_t>>> primitiveIndices;

        for (size_t i = 0;
             i < ceil(static_cast<float>(vertexAccessorIndexBatches.size()) / static_cast<float>(VERTEX_BATCH_SIZE));
             i++) {
            primitiveVertices.emplace_back(
                    loadVerticesByPrimitive(model, primitives, vertexAccessorIndexBatches, i * VERTEX_BATCH_SIZE));
        }

        for (size_t i = 0; i < primitives.size(); i++) {
            std::cout << "Adding primitive" << std::endl;
            primitiveIndices.emplace_back(loadIndicesByPrimitive(model, primitives[i], vertexOffsets[i]));
        }

        std::cout << "Done adding primitives" << std::endl;

        object.vertices = flatten(std::move(primitiveVertices)).get();
        object.indices = flatten(std::move(primitiveIndices)).get();

        return primitiveLookup;
    }

    std::future<std::vector<Vertex>> GLTFLoader::loadVerticesByPrimitive(
            std::shared_ptr<tinygltf::Model> model,
            const std::vector<tinygltf::Primitive *> &primitives,
            const std::vector<std::pair<size_t, size_t>> &primitiveIndexPairs,
            uint32_t startingIndex
    ) {
        return std::async(std::launch::async, [&, model = std::move(model), startingIndex] {
            std::vector<Vertex> vertices;
            vertices.reserve(VERTEX_BATCH_SIZE);
            const auto indexEnd = std::min(
                    primitiveIndexPairs.size(),
                    startingIndex + static_cast<size_t>(VERTEX_BATCH_SIZE)
            );

            for (size_t i = startingIndex; i < indexEnd; i++) {
                const auto &attribute = primitiveIndexPairs[i];
                const auto &primitive = primitives[attribute.second];
                const auto vertex = gltf::loader::vertex::generateVertexByPrimitive(model, *primitive, attribute.first);

                vertices.emplace_back(vertex);
            }

            return vertices;
        });
    }

    template<typename T>
    void loadIndices(
            const tinygltf::Accessor &indexAccessor,
            const tinygltf::BufferView &indexBufferView,
            tinygltf::Buffer &indexBuffer,
            std::vector<uint32_t> &indices,
            uint32_t vertexStart
    ) {
        auto data = std::span<T>(
                reinterpret_cast<T *>(&indexBuffer.data[indexAccessor.byteOffset + indexBufferView.byteOffset]),
                indexAccessor.count
        );

        for (const auto &element : data) {
            indices.emplace_back(static_cast<uint32_t>(element) + vertexStart);
        }
    }

    std::future<std::vector<uint32_t>> GLTFLoader::loadIndicesByPrimitive(
            const std::shared_ptr<tinygltf::Model> &model,
            const tinygltf::Primitive *primitive,
            uint32_t vertexStart
    ) {
        return std::async(std::launch::async, [model = model, primitive, vertexStart] {
            std::vector<uint32_t> indices;

            if (primitive->indices == -1) {
                // Model has no indices.
                return indices;
            }

            const tinygltf::Accessor &indexAccessor = model->accessors[primitive->indices];
            const tinygltf::BufferView &indexBufferView = model->bufferViews[indexAccessor.bufferView];
            tinygltf::Buffer &indexBuffer = model->buffers[indexBufferView.buffer];
            indices.reserve(indexAccessor.count);

            switch (indexAccessor.componentType) {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                    loadIndices<uint32_t>(indexAccessor, indexBufferView, indexBuffer, indices, vertexStart);
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                    loadIndices<uint16_t>(indexAccessor, indexBufferView, indexBuffer, indices, vertexStart);
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                    loadIndices<uint8_t>(indexAccessor, indexBufferView, indexBuffer, indices, vertexStart);
                    break;
                }
                default: {
                    throw std::runtime_error("Unsupported glTF index component type");
                }
            }

            return indices;
        });
    }

    namespace gltf::animation {
        std::vector<std::unique_ptr<Animation>> createFromGLTF(
                const std::string &filename,
                const Object &object
        ) {
            tinygltf::TinyGLTF loader;
            auto model = std::make_shared<tinygltf::Model>();
            std::string error;
            std::string warning;

            bool isAnimationLoaded = false;

            if (endsWith(filename, EXTENSION_GLB)) {
                isAnimationLoaded = loader.LoadBinaryFromFile(model.get(), &error, &warning, filename);
            } else {
                isAnimationLoaded = loader.LoadASCIIFromFile(model.get(), &error, &warning, filename);
            }

            if (!isAnimationLoaded) {
                throw std::runtime_error("Could not load glTF animation");
            }

            return loadAnimations(*model, object.getNodes());
        }
    }  // namespace gltf::animation
} // namespace pvk
