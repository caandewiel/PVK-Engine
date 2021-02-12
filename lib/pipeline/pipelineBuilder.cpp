//
//  pipeline.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 16/01/2021.
//

#include "pipelineBuilder.hpp"

namespace pvk::pipeline {
    Builder::Builder(const vk::RenderPass &renderPass,
                     const vk::PipelineLayout &pipelineLayout) {
        this->pipelineCreateInfo.setLayout(pipelineLayout);
        this->pipelineCreateInfo.setRenderPass(renderPass);

        this->initialize();
    }

    Builder::~Builder() = default;

    void Builder::initialize() {
        this->pipelineCreateInfo.setPRasterizationState(&this->rasterizationState);
//            this->pipelineCreateInfo.setPDynamicState(&this->dynamicState);
        this->pipelineCreateInfo.setPViewportState(&this->viewportState);
        this->pipelineCreateInfo.setPMultisampleState(&this->multisampleState);
        this->pipelineCreateInfo.setPColorBlendState(&this->colorBlendState);
        this->pipelineCreateInfo.setPDepthStencilState(&this->depthStencilState);
        this->pipelineCreateInfo.setPVertexInputState(&this->vertexInputState);
        this->pipelineCreateInfo.setPInputAssemblyState(&this->inputAssemblyState);

        this->depthStencilState.depthTestEnable = true;
        this->depthStencilState.depthWriteEnable = true;
        this->depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;

        this->colorBlendAttachmentState.colorWriteMask =
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
                vk::ColorComponentFlagBits::eA;
        this->colorBlendAttachmentState.blendEnable = VK_FALSE;

        this->colorBlendState.logicOpEnable = VK_FALSE;
        this->colorBlendState.logicOp = vk::LogicOp::eCopy;

        this->rasterizationState.setCullMode(vk::CullModeFlagBits::eBack);
        this->rasterizationState.setLineWidth(1.0f);
    }

    void Builder::update() {
        this->pipelineCreateInfo.setStages(this->shaderStages);

        std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        this->dynamicState.setDynamicStates(dynamicStates);

        this->inputAssemblyState.setTopology(vk::PrimitiveTopology::eTriangleList);

        this->colorBlendState.setAttachments(this->colorBlendAttachmentState);

        this->vertexInputState.setVertexBindingDescriptions(this->bindingDescriptions);
        this->vertexInputState.setVertexAttributeDescriptions(this->attributeDescriptions);

        this->viewportState.setViewports(this->viewports);
        this->viewportState.setScissors(this->scissors);
    }

    vk::Pipeline Builder::create(const vk::PipelineCache &cache) {
        this->update();

        return pvk::Context::getLogicalDevice().createGraphicsPipeline(nullptr, this->pipelineCreateInfo);
    }

    vk::Pipeline Builder::create() {
        return this->create(this->pipelineCache);
    }
}
