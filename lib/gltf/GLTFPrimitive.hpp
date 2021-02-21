//
//  PvkGLTFPrimitive.hpp
//  PVK
//
//  Created by Christian aan de Wiel on 10/01/2021.
//

#ifndef PvkGLTFPrimitive_hpp
#define PvkGLTFPrimitive_hpp

#include <cstdio>
#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "../util/util.hpp"

namespace pvk::gltf
{
class Primitive : pvk::util::NoCopy
{
  public:
    Primitive(uint32_t startVertex, uint32_t startIndex, uint32_t vertexCount, uint32_t indexCount);
    Primitive();

    Primitive(Primitive &&other) = default;
    Primitive &operator=(Primitive &&other) = default;

    ~Primitive();

  private:
    struct Material
    {
        glm::vec4 baseColorFactor;
        float metallicFactor;
        float roughnessFactor;
    } material{{1.0F, 1.0F, 1.0F, 1.0F}, 0.0F, 1.0F};

    uint32_t startIndex{};
    uint32_t startVertex{};
    uint32_t indexCount{};
    uint32_t vertexCount{};

    std::vector<vk::UniqueDescriptorSet> descriptorSets{};

public:
    [[nodiscard]] const Material &getMaterial() const;
    [[nodiscard]] uint32_t getStartIndex() const;
    [[nodiscard]] uint32_t getStartVertex() const;
    [[nodiscard]] uint32_t getIndexCount() const;
    [[nodiscard]] uint32_t getVertexCount() const;

    void setDescriptorSets(std::vector<vk::UniqueDescriptorSet> &&descriptorSets);
    [[nodiscard]] const std::vector<vk::UniqueDescriptorSet> &getDescriptorSets() const;
};
} // namespace pvk::gltf

#endif /* PvkGLTFPrimitive_hpp */
