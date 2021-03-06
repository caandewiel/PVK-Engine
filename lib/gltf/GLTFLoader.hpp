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

#define VERTEX_BATCH_SIZE 8000

#include <cmath>

#include <cstring>
#include <execution>
#include <future>
#include <glm/gtc/type_ptr.hpp>
#include <span>
#include <sstream>
#include <string>
#include <thread>

#include "tiny_gltf.h"

#include "../buffer/buffer.hpp"
#include "../context/context.hpp"
#include "GLTFAnimation.hpp"
#include "GLTFMaterial.hpp"
#include "GLTFNode.hpp"
#include "GLTFObject.hpp"
#include "GLTFPrimitive.hpp"
#include "GLTFSkin.hpp"

namespace pvk {
    class GLTFLoader {
    public:
        static std::unique_ptr<gltf::Object> loadObject(vk::Queue &graphicsQueue, const std::string &filePath);

        static std::vector<std::vector<std::shared_ptr<gltf::Primitive>>> loadPrimitives(
                const std::shared_ptr<tinygltf::Model> &model,
                const vk::Queue &graphicsQueue,
                gltf::Object &object
        );

        static auto loadVerticesByPrimitive(std::shared_ptr<tinygltf::Model> model,
                                            const std::vector<tinygltf::Primitive *> &primitives,
                                            const std::vector<std::pair<size_t, size_t>> &primitiveIndexPairs,
                                            uint32_t startingIndex) -> std::future<std::vector<Vertex>>;

        static auto loadIndicesByPrimitive(const std::shared_ptr<tinygltf::Model> &model,
                                           const tinygltf::Primitive *primitive,
                                           uint32_t vertexStart) -> std::future<std::vector<uint32_t>>;

        static auto initializePrimitiveLookupTable(std::vector<std::shared_ptr<gltf::Node>> &nodes)
        -> std::map<uint32_t, std::vector<std::weak_ptr<gltf::Primitive>>>;
    };

    namespace gltf::animation {
        std::vector<std::unique_ptr<gltf::Animation>> createFromGLTF(const std::string &filename,
                                                                     const gltf::Object &object);
    }
} // namespace pvk

#endif /* PvkGLTFLoader_hpp */
