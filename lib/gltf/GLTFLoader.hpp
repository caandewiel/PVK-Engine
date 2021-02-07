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

#include <cstdio>
#include <string>
#include <cstring>
#include <glm/gtc/type_ptr.hpp>

#include "tiny_gltf.h"

#include "../context/context.hpp"
#include "../buffer/buffer.hpp"
#include "GLTFObject.hpp"
#include "GLTFPrimitive.hpp"
#include "GLTFNode.hpp"
#include "GLTFAnimation.hpp"
#include "GLTFSkin.hpp"

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

        static void loadMaterial(tinygltf::Model &model,
                                 gltf::Primitive *primitive,
                                 uint32_t materialIndex);

    private:
        static const float *loadVertexPositionBuffer(const tinygltf::Model &model,
                                                     const tinygltf::Primitive &primitive);

        static const float *loadVertexNormalBuffer(const tinygltf::Model &model,
                                                   const tinygltf::Primitive &primitive);

        static const float *loadVertexColorBuffer(const tinygltf::Model &model,
                                                  const tinygltf::Primitive &primitive);

        static const float *loadVertexUV0Buffer(const tinygltf::Model &model,
                                                const tinygltf::Primitive &primitive);

        static const float *loadVertexUV1Buffer(const tinygltf::Model &model,
                                                const tinygltf::Primitive &primitive);

        static const uint8_t *loadVertexJointBuffer(const tinygltf::Model &model,
                                                     const tinygltf::Primitive &primitive);

        static const float *loadVertexWeightBuffer(const tinygltf::Model &model,
                                                   const tinygltf::Primitive &primitive);

        static int getVertexPositionByteStride(const tinygltf::Model &model,
                                               const tinygltf::Primitive &primitive);

        static int getVertexNormalByteStride(const tinygltf::Model &model,
                                             const tinygltf::Primitive &primitive);

        static int getVertexJointByteStride(const tinygltf::Model &model,
                                            const tinygltf::Primitive &primitive,
                                            const uint8_t *&buffer);

        static int getVertexWeightByteStride(const tinygltf::Model &model,
                                             const tinygltf::Primitive &primitive);

        static int getVertexColorType(const tinygltf::Model &model,
                                      const tinygltf::Primitive &primitive);

        static void loadVertexPosition(Vertex &vertex,
                                       const float *bufferPosition,
                                       const int &positionByteStride,
                                       const size_t &index);

        static void loadVertexNormal(Vertex &vertex,
                                     const float *bufferNormal,
                                     const int &normalByteStride,
                                     const size_t &index);

        static void loadVertexUV0(Vertex &vertex,
                                  const float *bufferUV0,
                                  const size_t &index);

        static void loadVertexUV1(Vertex &vertex,
                                  const float *bufferUV1,
                                  const size_t &index);

        static void loadVertexJoint(Vertex &vertex,
                                    const uint8_t *bufferJoint,
                                    const int &jointByteStride,
                                    const size_t &index);

        static void loadVertexWeight(Vertex &vertex,
                                     const float *bufferWeight,
                                     const int &weightByteStride,
                                     const size_t &index);

        static void loadVertexColor(Vertex &vertex,
                                    const float *bufferColor,
                                    const int &type,
                                    const size_t &index);

        static void loadIndices(std::vector<uint32_t> &indices,
                                tinygltf::Model &model,
                                tinygltf::Primitive &primitive,
                                uint32_t &vertexStart);

        static std::vector<gltf::Animation *> loadAnimations(tinygltf::Model &model,
                                                             std::map<uint32_t, gltf::Node *> &nodeLookup);
    };
}

#endif /* PvkGLTFLoader_hpp */
