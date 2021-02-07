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
        Pipeline(vk::RenderPass &renderPass,
                 vk::Extent2D &swapChainExtent,
                 pvk::Shader *shader,
                 vk::CullModeFlags cullModeFlags = vk::CullModeFlagBits::eBack,
                 bool enableDepth = true);
        ~Pipeline();
        void registerObject(Object *object);
        void registerObject(AssimpObject *object);
        void registerTexture(Texture *texture, uint32_t binding);
        void prepareForRenderStage();
        void bindToCommandBuffer(vk::CommandBuffer& commandBuffer);
        void render(pvk::Camera &camera, vk::CommandBuffer &commandBuffer, uint32_t swapChainIndex);
        
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

        void addWriteDescriptorSetUniformBuffer(std::vector<vk::WriteDescriptorSet> &writeDescriptorSets,
                                                pvk::AssimpMesh *mesh,
                                                vk::DescriptorSetLayoutBinding &descriptor,
                                                uint32_t i);

        void addWriteDescriptorSetCombinedImageSampler(std::vector<vk::WriteDescriptorSet> &writeDescriptorSets,
                                                       pvk::AssimpMesh *mesh,
                                                       vk::DescriptorSetLayoutBinding &descriptor,
                                                       uint32_t i);

        std::vector<Object*> objects {};
        std::vector<AssimpObject*> assimpObjects {};
        std::map<uint32_t, Texture*> textures;
        std::vector<vk::UniqueDescriptorSet> descriptorSets {};
        
        vk::Extent2D swapChainExtent;
        vk::RenderPass renderPass;
        vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack;
        vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
        vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
        vk::DescriptorPool descriptorPool;
        vk::DescriptorSetLayout descriptorSetLayout;
        
        pvk::Shader* shader;
    };
}

#endif /* pipeline_hpp */
