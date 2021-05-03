//
// Created by Christian aan de Wiel on 22/02/2021.
//

#ifndef PVK_DRAWABLE_H
#define PVK_DRAWABLE_H

#include <map>
#include <vulkan/vulkan.hpp>

#include "../buffer/buffer.hpp"
#include "../util/util.hpp"

namespace pvk
{
enum DrawableType
{
    DRAWABLE_NODE,
    DRAWABLE_PRIMITIVE
};

class Drawable : pvk::util::NoCopy
{
public:
    Drawable() = default;
    virtual ~Drawable() = default;
    Drawable(Drawable &&other) = default;
    Drawable &operator=(Drawable &&other) = default;

    [[nodiscard]] std::vector<vk::DescriptorSet> getDescriptorSetsBySwapChainIndex(uint32_t swapChainIndex) const;

    // DescriptorBufferInfo
    [[nodiscard]] const std::map<uint32_t, std::map<uint32_t, std::vector<vk::DescriptorBufferInfo>>>
        &getDescriptorBuffersInfo() const;
    [[nodiscard]] const vk::DescriptorBufferInfo &getDescriptorBufferInfo(uint32_t descriptorSetIndex,
                                                                          uint32_t bindingIndex,
                                                                          uint32_t swapChainIndex);

    // DescriptorSet
    [[nodiscard]] const std::map<uint32_t, std::vector<vk::UniqueDescriptorSet>> &getDescriptorSets() const;
    [[nodiscard]] std::map<uint32_t, std::vector<vk::UniqueDescriptorSet>> &getDescriptorSets();
    [[nodiscard]] const std::vector<vk::UniqueDescriptorSet> &getDescriptorSets(uint32_t descriptorSetIndex) const;
    [[nodiscard]] std::vector<vk::UniqueDescriptorSet> &getDescriptorSets(uint32_t descriptorSetIndex);
    [[nodiscard]] const vk::UniqueDescriptorSet &getDescriptorSet(uint32_t descriptorSetIndex,
                                                                  uint32_t swapChainImageIndex) const;
    void initializeDescriptorSets(vk::DescriptorSetAllocateInfo &descriptorSetAllocateInfo,
                                  uint32_t descriptorSetIndex);

    // Buffer
    [[nodiscard]] const vk::UniqueBuffer &getUniformBuffer(uint32_t descriptorSetIndex,
                                                           uint32_t bindingIndex,
                                                           uint32_t swapChainIndex) const;
    [[nodiscard]] vk::UniqueBuffer &getUniformBuffer(uint32_t descriptorSetIndex,
                                                     uint32_t bindingIndex,
                                                     uint32_t swapChainIndex);
    [[nodiscard]] std::vector<vk::UniqueBuffer> &getUniformBuffers(uint32_t descriptorSetIndex, uint32_t bindingIndex);

    // DeviceMemory
    [[nodiscard]] const vk::UniqueDeviceMemory &getUniformBufferMemory(uint32_t descriptorSetIndex,
                                                                       uint32_t bindingIndex,
                                                                       uint32_t swapChainIndex) const;
    [[nodiscard]] vk::UniqueDeviceMemory &getUniformBufferMemory(uint32_t descriptorSetIndex,
                                                                 uint32_t bindingIndex,
                                                                 uint32_t swapChainIndex);
    [[nodiscard]] std::vector<vk::UniqueDeviceMemory> &getUniformBuffersMemory(uint32_t descriptorSetIndex,
                                                                               uint32_t bindingIndex);

    [[nodiscard]] virtual constexpr DrawableType getType() const = 0;

    void addUniformBufferToDescriptorSet(const vk::DescriptorSetLayoutBinding &descriptor,
                                         size_t uniformBufferSize,
                                         uint32_t descriptorSetIndex,
                                         uint32_t swapChainImageIndex);

    void setDescriptorBufferInfo(vk::DescriptorBufferInfo &&descriptorBufferInfo,
                                 uint32_t descriptorSetIndex,
                                 uint32_t bindingIndex,
                                 uint32_t swapChainIndex);

protected:
    std::map<uint32_t, std::map<uint32_t, std::vector<vk::UniqueBuffer>>> uniformBuffers;
    std::map<uint32_t, std::map<uint32_t, std::vector<vk::UniqueDeviceMemory>>> uniformBuffersMemory;
    std::map<uint32_t, std::map<uint32_t, std::vector<vk::DescriptorBufferInfo>>> descriptorBuffersInfo;

    std::map<uint32_t, std::vector<vk::UniqueDescriptorSet>> descriptorSets;
};
} // namespace pvk

#endif // PVK_DRAWABLE_H
