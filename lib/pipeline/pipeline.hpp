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

namespace pvk {
    class Pipeline : pvk::util::NoCopy {
    public:
        Pipeline(vk::UniquePipeline vulkanPipeline, vk::UniquePipelineLayout pipelineLayout)
                : vulkanPipeline(std::move(vulkanPipeline)), pipelineLayout(std::move(pipelineLayout)) {};

        Pipeline(Pipeline &&other) = default;

        Pipeline &operator=(Pipeline &&other) = default;

        ~Pipeline();

        void setUniformBufferSize(uint8_t descriptorSetIndex, uint8_t descriptorSetBindingIndex, size_t size);

        void registerObject(const std::shared_ptr<Object> &object);

        void registerTexture(const std::shared_ptr<Texture> &texture, uint8_t descriptorSetIndex, uint8_t binding);

        void prepare();

        enum DescriptorSetVisibility {
            OBJECT, NODE, PRIMITIVE
        };

    private:
        void addWriteDescriptorSetUniformBuffer(std::vector<vk::WriteDescriptorSet> &writeDescriptorSets,
                                                Drawable &drawable,
                                                const vk::DescriptorSetLayoutBinding &descriptor,
                                                uint32_t descriptorSetIndex,
                                                uint32_t swapChainImageIndex) const;

        void addWriteDescriptorSetCombinedImageSampler(std::vector<vk::WriteDescriptorSet> &writeDescriptorSets,
                                                       const Drawable &drawable,
                                                       const vk::DescriptorSetLayoutBinding &descriptor,
                                                       uint32_t descriptorSetIndex,
                                                       uint32_t swapChainImageIndex);

        vk::UniquePipeline vulkanPipeline;
    public:
        [[nodiscard]] const vk::UniquePipeline &getVulkanPipeline() const;

        [[nodiscard]] const vk::UniquePipelineLayout &getPipelineLayout() const;

    private:
        vk::UniquePipelineLayout pipelineLayout;

        std::vector<std::shared_ptr<Object>> objects;
        std::vector<std::map<uint32_t, std::weak_ptr<Texture>>> textures;

        std::vector<DescriptorSetVisibility> descriptorSetVisibilities;
        std::vector<vk::UniqueDescriptorSetLayout> descriptorSetLayouts;
        std::vector<std::vector<vk::DescriptorSetLayoutBinding>> descriptorSetLayoutBindingsLookup;
        std::unordered_map<uint8_t, std::unordered_map<uint8_t, size_t>> descriptorSetLayoutBindingSizesLookup;
        std::vector<vk::UniqueDescriptorPool> descriptorPools;

    public:
        void setDescriptorSetVisibilities(std::vector<DescriptorSetVisibility> &&newDescriptorSetVisibilities);

        void setDescriptorSetLayouts(std::vector<vk::UniqueDescriptorSetLayout> &&newDescriptorSetLayouts);

        void setDescriptorSetLayoutBindingsLookup(
                std::vector<std::vector<vk::DescriptorSetLayoutBinding>> &&newDescriptorSetLayoutBindingsLookup);

        const std::vector<vk::WriteDescriptorSet> &
        initializeDescriptorSet(std::vector<vk::WriteDescriptorSet> &writeDescriptorSets, Drawable &drawable);

        template <vk::DescriptorType T>
        uint32_t getNumberOfResources(uint32_t descriptorSetLayoutIndex);

        void initializeDescriptorPools();

        uint32_t getNumberOfNodes();

        uint32_t getNumberOfPrimitives();

        [[nodiscard]] size_t getBindingSize(uint32_t descriptorSetIndex, uint32_t descriptorSetBinding) const {
            return this->descriptorSetLayoutBindingSizesLookup.at(descriptorSetIndex).at(descriptorSetBinding);
        }

        std::vector<vk::WriteDescriptorSet> getWriteDescriptorSets();

        void initializeDescriptorSets(std::shared_ptr<Object> &object);
    };
}  // namespace pvk

#endif /* pipeline_hpp */
