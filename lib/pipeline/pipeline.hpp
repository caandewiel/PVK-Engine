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
        Pipeline() = default;
        ~Pipeline();
        void setUniformBufferSize(uint8_t descriptorSetIndex, uint8_t descriptorSetBindingIndex, size_t size);
        void registerObject(Object *object);
        void registerTexture(Texture *texture, uint32_t binding);
        void prepare();

        vk::Pipeline vulkanPipeline;
        vk::PipelineLayout pipelineLayout;
        
    private:
        void addWriteDescriptorSetUniformBuffer(std::vector<vk::WriteDescriptorSet> &writeDescriptorSets,
                                                pvk::gltf::Node *node,
                                                vk::DescriptorSetLayoutBinding &descriptor,
                                                uint32_t i);
        
        void addWriteDescriptorSetCombinedImageSampler(std::vector<vk::WriteDescriptorSet> &writeDescriptorSets,
                                                       pvk::gltf::Node *node,
                                                       vk::DescriptorSetLayoutBinding &descriptor,
                                                       uint32_t i);

        std::vector<Object*> objects {};
        std::map<uint32_t, Texture*> textures;

        vk::DescriptorPool descriptorPool;
        vk::DescriptorSetLayout descriptorSetLayout;
        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
        std::vector<std::vector<vk::DescriptorSetLayoutBinding>> descriptorSetLayoutBindingsLookup;
        std::unordered_map<uint8_t, std::unordered_map<uint8_t, size_t>> descriptorSetLayoutBindingSizesLookup;
    public:
        void setDescriptorSetLayouts(const std::vector<vk::DescriptorSetLayout> &newDescriptorSetLayouts);

        void setDescriptorSetLayoutBindingsLookup(
                const std::vector<std::vector<vk::DescriptorSetLayoutBinding>> &newDescriptorSetLayoutBindingsLookup);
    };
}

#endif /* pipeline_hpp */
