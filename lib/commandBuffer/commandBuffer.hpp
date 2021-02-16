//
//  commandBuffer.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 22/01/2021.
//

#ifndef commandBuffer_h
#define commandBuffer_h

#include <cstdio>
#include <vulkan/vulkan.hpp>

#include "../pipeline/pipeline.hpp"
#include "../gltf/GLTFNode.hpp"
#include "../io/io.hpp"

namespace pvk {
    class CommandBuffer {
    public:
        CommandBuffer(vk::CommandBuffer* commandBuffer, uint32_t swapchainIndex) : commandBuffer(commandBuffer), swapchainIndex(swapchainIndex) {};
        
        void drawNode(const Pipeline &pipeline, const gltf::Object &object, const gltf::Node &node) {
            this->commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getVulkanPipeline().get());
            this->commandBuffer->bindVertexBuffers(0, object.vertexBuffer, {0});
            if (object.indices.empty()) {
                this->commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                                        pipeline.getPipelineLayout().get(), 0, 1,
                                                        &node.descriptorSets[this->swapchainIndex].get(), 0, nullptr);
                for (auto &primitive : object.primitiveLookup.at(node.nodeIndex)) {
                    this->commandBuffer->draw(primitive->vertexCount, 1, primitive->startVertex, 0);
                }
            } else {
                this->commandBuffer->bindIndexBuffer(object.indexBuffer, 0, vk::IndexType::eUint32);
                this->commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                                        pipeline.getPipelineLayout().get(), 0, 1,
                                                        &node.descriptorSets[this->swapchainIndex].get(), 0, nullptr);

                for (auto &primitive : node.primitives) {
                    this->commandBuffer->drawIndexed(primitive->indexCount, 1, primitive->startIndex, 0, 0);
                }
            }
        }

    private:
        vk::CommandBuffer* commandBuffer;
        uint32_t swapchainIndex;
    };
}

#endif /* commandBuffer_h */
