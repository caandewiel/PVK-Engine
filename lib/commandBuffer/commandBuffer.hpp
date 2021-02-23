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

#include "../gltf/GLTFNode.hpp"
#include "../pipeline/pipeline.hpp"

namespace pvk
{
class CommandBuffer
{
  public:
    CommandBuffer(vk::CommandBuffer *commandBuffer, uint32_t swapchainIndex)
        : commandBuffer(commandBuffer), swapchainIndex(swapchainIndex){};

    void drawNode(const Pipeline &pipeline, const gltf::Object &object, const gltf::Node &node)
    {
        this->commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getVulkanPipeline().get());
        this->commandBuffer->bindVertexBuffers(0, object.vertexBuffer.get(), {0});
        if (object.indices.empty())
        {
            this->commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                                    pipeline.getPipelineLayout().get(),
                                                    0,
                                                    1,
                                                    node.getDescriptorSetsBySwapChainIndex(this->swapchainIndex).data(),
                                                    0,
                                                    nullptr);
            for (auto &primitive : object.primitiveLookup.at(node.nodeIndex))
            {
                this->commandBuffer->draw(primitive.lock()->getVertexCount(), 1, primitive.lock()->getStartVertex(), 0);
            }
        }
        else
        {
            this->commandBuffer->bindIndexBuffer(object.indexBuffer.get(), 0, vk::IndexType::eUint32);
            this->commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                                    pipeline.getPipelineLayout().get(),
                                                    0,
                                                    node.getDescriptorSets().size(),
                                                    node.getDescriptorSetsBySwapChainIndex(this->swapchainIndex).data(),
                                                    0,
                                                    nullptr);

            for (auto &primitive : node.primitives)
            {
                if (primitive->getDescriptorSets().empty()) {
                    // Primitive has no descriptor set.
                } else {
                    this->commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                                            pipeline.getPipelineLayout().get(),
                                                            1,
                                                            primitive->getDescriptorSets().size(),
                                                            primitive->getDescriptorSetsBySwapChainIndex(this->swapchainIndex).data(),
                                                            0,
                                                            nullptr);
                }
                this->commandBuffer->drawIndexed(primitive->getIndexCount(), 1, primitive->getStartIndex(), 0, 0);
            }
        }
    }

  private:
    vk::CommandBuffer *commandBuffer;
    uint32_t swapchainIndex;
};
} // namespace pvk

#endif /* commandBuffer_h */
