//
//  PvkGLTFLoader.hpp
//  PVK
//
//  Created by Christian aan de Wiel on 02/01/2021.
//

#ifndef PvkGLTFLoader_hpp
#define PvkGLTFLoader_hpp

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define EXTENSION_GLB ".glb"
#define FIELD_VERTEX_POSITION "POSITION"
#define FIELD_VERTEX_NORMAL "NORMAL"
#define FIELD_VERTEX_COLOR_0 "COLOR_0"
#define FIELD_VERTEX_TEXCOORD_0 "TEXCOORD_0"
#define FIELD_VERTEX_TEXCOORD_1 "TEXCOORD_1"
#define FIELD_VERTEX_JOINTS_0 "JOINTS_0"
#define FIELD_VERTEX_WEIGHTS_0 "WEIGHTS_0"

#define VERTEX_BATCH_SIZE 1024

#include <cstdio>
#include <string>
#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <future>
#include <execution>
#include <thread>
#include <span>
#include <cmath>

#include "tiny_gltf.h"

#include "../context/context.hpp"
#include "../buffer/buffer.hpp"
#include "GLTFObject.hpp"
#include "GLTFPrimitive.hpp"
#include "GLTFNode.hpp"
#include "GLTFAnimation.hpp"
#include "GLTFSkin.hpp"
#include "GLTFMaterial.hpp"

namespace pvk {
    class GLTFLoader {
    public:
        static std::unique_ptr<gltf::Object> loadObject(vk::Queue &graphicsQueue,
                                                        const std::string &filePath);

        static auto loadPrimitives(const std::shared_ptr<tinygltf::Model> &model,
                                   vk::Queue &graphicsQueue,
                                   gltf::Object &object)
        -> std::vector<std::vector<std::unique_ptr<gltf::Primitive>>>;


        static std::future<std::vector<Vertex>> loadVerticesByPrimitive(std::shared_ptr<tinygltf::Model> model,
                                                                        const std::vector<tinygltf::Primitive *> &primitives,
                                                                        const std::vector<std::pair<size_t, size_t>> &primitiveIndexPairs,
                                                                        uint32_t startingIndex);

        static std::future<std::vector<uint32_t>> loadIndicesByPrimitive(const std::shared_ptr<tinygltf::Model> &model,
                                                                         const tinygltf::Primitive *primitive,
                                                                         uint32_t vertexStart);

        static auto loadNodes(const std::shared_ptr<tinygltf::Model> &model,
                              std::vector<std::vector<std::unique_ptr<gltf::Primitive>>> &primitiveLookup,
                              vk::Queue &graphicsQueue,
                              gltf::Object &object)
        -> std::vector<std::shared_ptr<gltf::Node>>;

        static auto loadNode(const std::shared_ptr<tinygltf::Model> &model,
                             std::vector<std::vector<std::unique_ptr<gltf::Primitive>>> &primitiveLookup,
                             uint32_t nodeIndex,
                             vk::Queue &graphicsQueue,
                             gltf::Object &object,
                             std::shared_ptr<gltf::Node> parent = nullptr)
        -> std::shared_ptr<gltf::Node>;

        static std::map<uint32_t, std::shared_ptr<gltf::Node>> initializeNodeLookupTable(
                std::vector<std::shared_ptr<gltf::Node>> &nodes);

        static std::map<uint32_t, std::vector<gltf::Primitive *>>
        initializePrimitiveLookupTable(std::vector<std::shared_ptr<gltf::Node>> &nodes);

        static std::vector<gltf::Skin *> loadSkins(const std::shared_ptr<tinygltf::Model> &model,
                                                   const std::map<uint32_t, std::shared_ptr<gltf::Node>> &nodeLookup);

        static void
        loadMaterials(const std::shared_ptr<tinygltf::Model> &model, vk::Queue &graphicsQueue, gltf::Object &object);

        static void loadMaterial(const std::shared_ptr<tinygltf::Model> &model,
                                 gltf::Primitive *primitive,
                                 uint32_t materialIndex);

    private:
        static void loadIndices(std::vector<uint32_t> &indices,
                                const std::shared_ptr<tinygltf::Model> &model,
                                tinygltf::Primitive &primitive,
                                uint32_t &vertexStart);

        static std::vector<gltf::Animation *> loadAnimations(const std::shared_ptr<tinygltf::Model> &model,
                                                             const std::map<uint32_t, std::shared_ptr<gltf::Node>> &nodeLookup);

        static void loadVertexPosition(const std::shared_ptr<tinygltf::Model> &model,
                                       const tinygltf::Primitive &primitive,
                                       Vertex &vertex,
                                       uint32_t index);

        static void loadVertexNormal(const std::shared_ptr<tinygltf::Model> &model,
                                     const tinygltf::Primitive &primitive,
                                     Vertex &vertex,
                                     uint32_t index);

        static void loadVertexColor(const std::shared_ptr<tinygltf::Model> &model,
                                    const tinygltf::Primitive &primitive,
                                    Vertex &vertex,
                                    uint32_t index);

        static void loadVertexUV0(const std::shared_ptr<tinygltf::Model> &model,
                                  const tinygltf::Primitive &primitive,
                                  Vertex &vertex,
                                  uint32_t index);

        static void loadVertexUV1(const std::shared_ptr<tinygltf::Model> &model,
                                  const tinygltf::Primitive &primitive,
                                  Vertex &vertex,
                                  uint32_t index);

        static void loadVertexJoints(const std::shared_ptr<tinygltf::Model> &model,
                                     const tinygltf::Primitive &primitive,
                                     Vertex &vertex,
                                     uint32_t index);

        static void loadVertexWeights(const std::shared_ptr<tinygltf::Model> &model,
                                      const tinygltf::Primitive &primitive,
                                      Vertex &vertex,
                                      uint32_t index);

        static auto createVertexBuffer(vk::Queue &graphicsQueue, const std::unique_ptr<gltf::Object> &object) -> std::future<void>;

        static auto createIndexBuffer(vk::Queue &graphicsQueue, const std::unique_ptr<gltf::Object> &object) -> std::future<void>;
    };
}

#endif /* PvkGLTFLoader_hpp */
