//
//  pipeline.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 21/01/2021.
//

#ifndef pipeline_hpp
#define pipeline_hpp

#include <cstdio>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "../context/context.hpp"
#include "../object/object.hpp"
#include "../gltf/GLTFObject.hpp"
#include "../shader/shader.hpp"
#include "../pipeline/pipelineBuilder.hpp"
#include "../camera/camera.hpp"
#include "../texture/texture.hpp"
#include "../io/io.hpp"

namespace pvk {
    class Pipeline {
    public:
        Pipeline(vk::UniquePipeline vulkanPipeline, vk::UniquePipelineLayout pipelineLayout)
                : vulkanPipeline(std::move(vulkanPipeline)), pipelineLayout(std::move(pipelineLayout)) {}

        ~Pipeline();

        void setUniformBufferSize(uint8_t descriptorSetIndex, uint8_t descriptorSetBindingIndex, size_t size);

        void registerObject(std::shared_ptr<Object> object);

        void registerTexture(Texture *texture, uint32_t binding);

        void prepare();

    private:
        void addWriteDescriptorSetUniformBuffer(std::vector<vk::WriteDescriptorSet> &writeDescriptorSets,
                                                gltf::Node &node,
                                                vk::DescriptorSetLayoutBinding &descriptor,
                                                uint32_t i);

        void addWriteDescriptorSetCombinedImageSampler(std::vector<vk::WriteDescriptorSet> &writeDescriptorSets,
                                                       const gltf::Node &node,
                                                       const vk::DescriptorSetLayoutBinding &descriptor,
                                                       uint32_t i);

        vk::UniquePipeline vulkanPipeline;
    public:
        [[nodiscard]] auto getVulkanPipeline() const -> const vk::UniquePipeline &;

        [[nodiscard]] auto getPipelineLayout() const -> const vk::UniquePipelineLayout &;

    private:
        vk::UniquePipelineLayout pipelineLayout;

        std::vector<std::shared_ptr<Object>> objects{};
        std::map<uint32_t, Texture *> textures;

        vk::UniqueDescriptorPool descriptorPool;
        std::vector<vk::UniqueDescriptorSetLayout> descriptorSetLayouts;
        std::vector<std::vector<vk::DescriptorSetLayoutBinding>> descriptorSetLayoutBindingsLookup;
        std::unordered_map<uint8_t, std::unordered_map<uint8_t, size_t>> descriptorSetLayoutBindingSizesLookup;
    public:
        void setDescriptorSetLayouts(std::vector<vk::UniqueDescriptorSetLayout> &newDescriptorSetLayouts);

        void setDescriptorSetLayoutBindingsLookup(
                const std::vector<std::vector<vk::DescriptorSetLayoutBinding>> &newDescriptorSetLayoutBindingsLookup);

        auto initializeDescriptorSet(uint32_t numberOfSwapchainImages,
                                     std::vector<vk::WriteDescriptorSet> &writeDescriptorSets,
                                     gltf::Node &node) -> std::vector<vk::WriteDescriptorSet> &;

        void getNumberOfResources(uint32_t &numberOfUniformBuffers, uint32_t &numberOfCombinedImageSamplers);
    };
}

#endif /* pipeline_hpp */
