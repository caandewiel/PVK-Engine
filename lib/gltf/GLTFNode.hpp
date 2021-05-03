//
//  PvkGLTFNode.hpp
//  PVK
//
//  Created by Christian aan de Wiel on 10/01/2021.
//

#ifndef PvkGLTFNode_hpp
#define PvkGLTFNode_hpp


#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "../buffer/buffer.hpp"
#include "../mesh/mesh.hpp"
#include "../util/util.hpp"
#include "Drawable.h"
#include "GLTFPrimitive.hpp"

namespace pvk::gltf
{
class Node : public Drawable
{
public:
    Node() = default;
    ~Node() = default;

    Node(Node &&other) = default;
    Node &operator=(Node &&other) = default;

    [[nodiscard]] glm::mat4 getGlobalMatrix() const;
    [[nodiscard]] glm::mat4 getLocalMatrix() const;
    [[nodiscard]] constexpr DrawableType getType() const override {
        return DrawableType::DRAWABLE_NODE;
    }

    std::vector<std::shared_ptr<Node>> children;
    std::vector<std::shared_ptr<Primitive>> primitives;
    std::unique_ptr<Mesh> mesh;
    std::weak_ptr<Node> parent;
    int32_t skinIndex = -1;
    int32_t nodeIndex = -1;
    std::string name;

    struct
    {
        glm::mat4 model;
        glm::mat4 localMatrix;
        glm::mat4 inverseBindMatrices[256];
        float jointCount;
    } bufferObject{};

    glm::vec3 translation = glm::vec3(0.0F);
    glm::vec3 scale = glm::vec3(1.0F);
    glm::mat4 rotation = glm::mat4(1.0F);
    glm::mat4 matrix = glm::mat4(1.0F);
};
} // namespace pvk::gltf

#endif /* PvkGLTFNode_hpp */
