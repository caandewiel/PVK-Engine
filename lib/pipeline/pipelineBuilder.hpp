//
//  pipeline.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 16/01/2021.
//  Based on: https://github.com/jherico/Vulkan/blob/cpp/base/vks/pipelines.hpp

#ifndef pipelineBuilder_hpp
#define pipelineBuilder_hpp

#include <cstdio>
#include <string>
#include <vulkan/vulkan.hpp>

#include "../context/context.hpp"
#include "../util/util.hpp"

namespace pvk::pipeline {
        class Builder {
        public:
            Builder(const vk::RenderPass &renderPass, const vk::UniquePipelineLayout &pipelineLayout);
            ~Builder();
            vk::UniquePipeline create(const vk::PipelineCache &cache);
            vk::UniquePipeline create();
            void update();
            void initialize();
            
            vk::PipelineCache pipelineCache;
            vk::RenderPass& renderPass{ pipelineCreateInfo.renderPass };
            vk::PipelineLayout& layout{ pipelineCreateInfo.layout };
            uint32_t& subpass{ pipelineCreateInfo.subpass };
            vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
            vk::PipelineRasterizationStateCreateInfo rasterizationState;
            vk::PipelineMultisampleStateCreateInfo multisampleState;
            vk::PipelineDepthStencilStateCreateInfo depthStencilState;
            vk::PipelineViewportStateCreateInfo viewportState;
            vk::PipelineDynamicStateCreateInfo dynamicState;
            vk::PipelineColorBlendStateCreateInfo colorBlendState;
            vk::PipelineVertexInputStateCreateInfo vertexInputState;
            vk::PipelineColorBlendAttachmentState colorBlendAttachmentState;
            std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
            
            // Needs to be set publicly
            std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
            std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
            std::vector<vk::Viewport> viewports;
            std::vector<vk::Rect2D> scissors;

            vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
        };
    }

#endif /* pipeline_hpp */
