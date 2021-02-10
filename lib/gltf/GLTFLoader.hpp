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

#include <cstdio>
#include <string>
#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <future>
#include <execution>

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
        static gltf::Object *loadObject(vk::Queue &graphicsQueue,
                                        const std::string &filePath);

        static std::vector<gltf::Node *> loadNodes(tinygltf::Model &model,
                                                   vk::Queue &graphicsQueue,
                                                   gltf::Object *&object);

        static gltf::Node *loadNode(tinygltf::Model &model,
                                    int nodeIndex,
                                    vk::Queue &graphicsQueue,
                                    gltf::Object *&object,
                                    gltf::Node *parent = nullptr);

        static std::map<uint32_t, gltf::Node *> initializeNodeLookupTable(std::vector<gltf::Node *> &nodes);

        static std::map<uint32_t, std::vector<gltf::Primitive *>>
        initializePrimitiveLookupTable(std::vector<gltf::Node *> &nodes);

        static std::map<uint32_t, std::vector<gltf::Skin *>>
        initializeSkinLookupTable(std::vector<gltf::Node *> &nodes);

        static std::vector<gltf::Skin *> loadSkins(tinygltf::Model &model,
                                                   std::map<uint32_t, gltf::Node *> &nodeLookup);

        static void loadMaterials(tinygltf::Model &model, vk::Queue &graphicsQueue, gltf::Object *object);

        static void loadMaterial(tinygltf::Model &model,
                                 gltf::Primitive *primitive,
                                 uint32_t materialIndex);

    private:
        static void loadIndices(std::vector<uint32_t> &indices,
                                tinygltf::Model &model,
                                tinygltf::Primitive &primitive,
                                uint32_t &vertexStart);

        static std::vector<gltf::Animation *> loadAnimations(tinygltf::Model &model,
                                                             std::map<uint32_t, gltf::Node *> &nodeLookup);

        static void loadVertexPosition(const tinygltf::Model &model,
                                       const tinygltf::Primitive &primitive,
                                       Vertex &vertex,
                                       uint32_t index);

        static void loadVertexNormal(const tinygltf::Model &model,
                                     const tinygltf::Primitive &primitive,
                                     Vertex &vertex,
                                     uint32_t index);

        static void loadVertexColor(const tinygltf::Model &model,
                                    const tinygltf::Primitive &primitive,
                                    Vertex &vertex,
                                    uint32_t index);

        static void loadVertexUV0(const tinygltf::Model &model,
                                  const tinygltf::Primitive &primitive,
                                  Vertex &vertex,
                                  uint32_t index);

        static void loadVertexUV1(const tinygltf::Model &model,
                                  const tinygltf::Primitive &primitive,
                                  Vertex &vertex,
                                  uint32_t index);

        static void loadVertexJoints(const tinygltf::Model &model,
                                     const tinygltf::Primitive &primitive,
                                     Vertex &vertex,
                                     uint32_t index);

        static void loadVertexWeights(const tinygltf::Model &model,
                                      const tinygltf::Primitive &primitive,
                                      Vertex &vertex,
                                      uint32_t index);
    };
}

#endif /* PvkGLTFLoader_hpp */
