//
//  buffer.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 16/01/2021.
//

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "buffer.hpp"

namespace pvk {
    namespace buffer {
        inline uint32_t findMemoryType(const uint32_t typeFilter,
                                       const vk::MemoryPropertyFlags properties)
        {
            vk::PhysicalDeviceMemoryProperties memProperties = Context::getPhysicalDevice().getMemoryProperties();

            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
                if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                    return i;
                }
            }

            throw std::runtime_error("Could not find suitable memory type.");
        }
        
        /**
         Creates a buffer on the provided device.
         */
        void create(const vk::DeviceSize size,
                    const vk::BufferUsageFlags usage,
                    const vk::MemoryPropertyFlags properties,
                    vk::Buffer &buffer,
                    vk::DeviceMemory &bufferMemory)
        {
            vk::BufferCreateInfo bufferInfo = {};
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = vk::SharingMode::eExclusive;

            try {
                buffer = Context::getLogicalDevice().createBuffer(bufferInfo);
            }
            catch (vk::SystemError err) {
                throw std::runtime_error("failed to create buffer!");
            }

            vk::MemoryRequirements memRequirements = Context::getLogicalDevice().getBufferMemoryRequirements(buffer);

            vk::MemoryAllocateInfo allocInfo = {};
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

            try {
                bufferMemory = Context::getLogicalDevice().allocateMemory(allocInfo);
            }
            catch (vk::SystemError err) {
                throw std::runtime_error("Failed to allocate buffer memory.");
            }

            Context::getLogicalDevice().bindBufferMemory(buffer, bufferMemory, 0);
        }
        
        /**
         Copies the contents from the source buffer to the target buffer.
         */
        void copy(vk::Queue &graphicsQueue,
                  vk::Buffer &srcBuffer,
                  vk::Buffer &dstBuffer,
                  vk::DeviceSize size)
        {
            vk::CommandBufferAllocateInfo allocInfo = {};
            allocInfo.level = vk::CommandBufferLevel::ePrimary;
            allocInfo.commandPool = Context::getCommandPool();
            allocInfo.commandBufferCount = 1;

            vk::CommandBuffer commandBuffer = Context::getLogicalDevice().allocateCommandBuffers(allocInfo)[0];

            vk::CommandBufferBeginInfo beginInfo = {};
            beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

            commandBuffer.begin(beginInfo);

                vk::BufferCopy copyRegion = {};
                copyRegion.srcOffset = 0; // Optional
                copyRegion.dstOffset = 0; // Optional
                copyRegion.size = size;
                commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

            commandBuffer.end();

            vk::SubmitInfo submitInfo = {};
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            graphicsQueue.submit(submitInfo, nullptr);
            graphicsQueue.waitIdle();

            Context::getLogicalDevice().freeCommandBuffers(Context::getCommandPool(), commandBuffer);
        }
        
        void copyToImage(const vk::CommandBuffer &commandBuffer,
                         const vk::Queue &graphicsQueue,
                         const vk::Buffer &buffer,
                         const vk::Image &image,
                         uint32_t width, uint32_t height)
        {
            vk::BufferImageCopy region = {
                0, 0, 0,
                {vk::ImageAspectFlagBits::eColor, 0, 0, NUMBER_OF_FACES_FOR_CUBE},
                {0, 0, 0},
                {width, height, 1},
            };

            commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
        }

        
        void update(vk::DeviceMemory bufferMemory, size_t bufferSize, void* data) {
            void* dataMapped;
            vkMapMemory(Context::getLogicalDevice(), bufferMemory, 0, bufferSize, 0, &dataMapped);
            memcpy(dataMapped, data, bufferSize);
            vkUnmapMemory(Context::getLogicalDevice(), bufferMemory);
        }
        
        namespace vertex {
            void create(vk::Queue &graphicsQueue,
                        vk::Buffer &buffer,
                        vk::DeviceMemory &bufferMemory,
                        std::vector<Vertex> &vertices)
            {
                vk::DeviceSize bufferSize = sizeof(vertices.front()) * vertices.size();

                vk::Buffer stagingBuffer;
                vk::DeviceMemory stagingBufferMemory;
                pvk::buffer::create(bufferSize,
                                    vk::BufferUsageFlagBits::eTransferSrc,
                                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                    stagingBuffer,
                                    stagingBufferMemory);
                void* data = Context::getLogicalDevice().mapMemory(stagingBufferMemory, 0, bufferSize);
                memcpy(data, vertices.data(), (size_t) bufferSize);
                Context::getLogicalDevice().unmapMemory(stagingBufferMemory);

                pvk::buffer::create(bufferSize,
                                    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                                    vk::MemoryPropertyFlagBits::eDeviceLocal,
                                    buffer,
                                    bufferMemory);
                
                pvk::buffer::copy(graphicsQueue, stagingBuffer, buffer, bufferSize);

                Context::getLogicalDevice().destroyBuffer(stagingBuffer);
                Context::getLogicalDevice().freeMemory(stagingBufferMemory);
            }
        }
        
        namespace index {
            void create(vk::Queue &graphicsQueue,
                        vk::Buffer &buffer,
                        vk::DeviceMemory &bufferMemory,
                        std::vector<uint32_t> &indices)
            {
                vk::DeviceSize bufferSize = sizeof(indices.front()) * indices.size();

                vk::Buffer stagingBuffer;
                vk::DeviceMemory stagingBufferMemory;
                pvk::buffer::create(bufferSize,
                                    vk::BufferUsageFlagBits::eTransferSrc,
                                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                    stagingBuffer,
                                    stagingBufferMemory);
                void* data = Context::getLogicalDevice().mapMemory(stagingBufferMemory, 0, bufferSize);
                memcpy(data, indices.data(), (size_t) bufferSize);
                Context::getLogicalDevice().unmapMemory(stagingBufferMemory);

                pvk::buffer::create(bufferSize,
                                    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                                    vk::MemoryPropertyFlagBits::eDeviceLocal,
                                    buffer,
                                    bufferMemory);
                
                pvk::buffer::copy(graphicsQueue, stagingBuffer, buffer, bufferSize);

                Context::getLogicalDevice().destroyBuffer(stagingBuffer);
                Context::getLogicalDevice().freeMemory(stagingBufferMemory);
            }
        }
        
        namespace texture {
            void create(const vk::Queue &graphicsQueue,
                        const tinygltf::Image &gltfImage,
                        pvk::Texture &texture)
            {
                vk::Buffer stagingBuffer;
                vk::DeviceMemory stagingBufferMemory;

                assert(gltfImage.component != 3);

                pvk::buffer::create(gltfImage.image.size(),
                                    vk::BufferUsageFlagBits::eTransferSrc,
                                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                    stagingBuffer,
                                    stagingBufferMemory);

                void* data = Context::getLogicalDevice().mapMemory(stagingBufferMemory, 0, gltfImage.image.size());
                memcpy(data, gltfImage.image.data(), gltfImage.image.size());
                Context::getLogicalDevice().unmapMemory(stagingBufferMemory);

                auto width = static_cast<uint32_t>(gltfImage.width);
                auto height = static_cast<uint32_t>(gltfImage.height);

                vk::BufferImageCopy bufferCopyRegion;
                bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                bufferCopyRegion.imageSubresource.layerCount = 1;
                bufferCopyRegion.imageExtent.depth = 1;
                bufferCopyRegion.imageSubresource.mipLevel = 0;
                bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
                bufferCopyRegion.imageExtent.width = width;
                bufferCopyRegion.imageExtent.height = height;

                pvk::image::create(width, height, 1, 1,
                                   vk::SampleCountFlagBits::e1,
                                   vk::Format::eR8G8B8A8Unorm,
                                   vk::ImageTiling::eOptimal,
                                   vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                                   vk::MemoryPropertyFlagBits::eDeviceLocal,
                                   {},
                                   texture.image, texture.imageMemory);

                auto commandBuffers = pvk::util::beginOneTimeCommandBuffer();

                pvk::image::transitionLayout(commandBuffers.front(), graphicsQueue, texture.image.get(), vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1, 1);

                pvk::buffer::copyToImage(commandBuffers.front(), graphicsQueue, stagingBuffer, texture.image.get(), width, height);

                pvk::image::transitionLayout(commandBuffers.front(), graphicsQueue, texture.image.get(), vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 1, 1);

                pvk::util::endSingleTimeCommands(commandBuffers, graphicsQueue);

                vk::SamplerCreateInfo samplerCreateInfo;
                samplerCreateInfo.magFilter = vk::Filter::eLinear;
                samplerCreateInfo.minFilter = vk::Filter::eLinear;
                samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
                samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
                samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
                samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
                // Max level-of-detail should match mip level count
                samplerCreateInfo.maxLod = static_cast<float>(1);
                // Only enable anisotropic filtering if enabled on the devicec
                samplerCreateInfo.maxAnisotropy = 1.0f;
                samplerCreateInfo.anisotropyEnable = VK_FALSE;
                samplerCreateInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
                texture.sampler = Context::getLogicalDevice().createSamplerUnique(samplerCreateInfo);

                texture.imageView = Context::getLogicalDevice().createImageViewUnique(vk::ImageViewCreateInfo{
                        {},
                        texture.image.get(),
                        vk::ImageViewType::e2D,
                        vk::Format::eR8G8B8A8Unorm,
                        {},
                        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
                });

                Context::getLogicalDevice().destroyBuffer(stagingBuffer);
                Context::getLogicalDevice().freeMemory(stagingBufferMemory);
            }

            void create(const vk::Queue &graphicsQueue,
                        const gli::texture_cube &textureCube,
                        pvk::Texture &texture)
            {
                vk::Buffer stagingBuffer;
                vk::DeviceMemory stagingBufferMemory;
                pvk::buffer::create(textureCube.size(),
                                    vk::BufferUsageFlagBits::eTransferSrc,
                                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                    stagingBuffer,
                                    stagingBufferMemory);
                
                void* data = Context::getLogicalDevice().mapMemory(stagingBufferMemory, 0, textureCube.size());
                memcpy(data, textureCube.data(), textureCube.size());
                Context::getLogicalDevice().unmapMemory(stagingBufferMemory);
                
                uint32_t width = static_cast<uint32_t>(textureCube.extent().x);
                uint32_t height = static_cast<uint32_t>(textureCube.extent().y);
                uint32_t mipLevels = static_cast<uint32_t>(textureCube.levels());
                uint32_t numberOfLayers = static_cast<uint32_t>(textureCube.layers());
                
                // Create buffer copy regions for all layers and all mip levels.
                std::vector<vk::BufferImageCopy> bufferCopyRegions;
                size_t offset = 0;
                
                vk::BufferImageCopy bufferCopyRegion;
                bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                bufferCopyRegion.imageSubresource.layerCount = 1;
                bufferCopyRegion.imageExtent.depth = 1;
                
                for (uint32_t layer = 0; layer < numberOfLayers; layer++) {
                    for (uint32_t level = 0; level < mipLevels; level++) {
                        auto image = textureCube[layer][level];
                        auto imageExtent = image.extent();
                        bufferCopyRegion.imageSubresource.mipLevel = level;
                        bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
                        bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(imageExtent.x);
                        bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(imageExtent.y);
                        bufferCopyRegion.bufferOffset = offset;
                        bufferCopyRegions.push_back(bufferCopyRegion);
                        
                        // Increase offset into staging buffer for next level / face
                        offset += image.size();
                    }
                }
                
                pvk::image::create(width, height, mipLevels, NUMBER_OF_FACES_FOR_CUBE,
                                   vk::SampleCountFlagBits::e1,
                                   vk::Format::eR8G8B8A8Unorm,
                                   vk::ImageTiling::eOptimal,
                                   vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                                   vk::MemoryPropertyFlagBits::eDeviceLocal,
                                   vk::ImageCreateFlagBits::eCubeCompatible,
                                   texture.image, texture.imageMemory);
                
                auto commandBuffers = pvk::util::beginOneTimeCommandBuffer();
                
                pvk::image::transitionLayout(commandBuffers.front(), graphicsQueue, texture.image.get(), vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mipLevels, NUMBER_OF_FACES_FOR_CUBE);

                pvk::buffer::copyToImage(commandBuffers.front(), graphicsQueue, stagingBuffer, texture.image.get(), width, height);
                
                pvk::image::transitionLayout(commandBuffers.front(), graphicsQueue, texture.image.get(), vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, mipLevels, NUMBER_OF_FACES_FOR_CUBE);
                
                pvk::util::endSingleTimeCommands(commandBuffers, graphicsQueue);
                
                vk::SamplerCreateInfo samplerCreateInfo;
                samplerCreateInfo.magFilter = vk::Filter::eLinear;
                samplerCreateInfo.minFilter = vk::Filter::eLinear;
                samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
                samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
                samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
                samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
                // Max level-of-detail should match mip level count
                samplerCreateInfo.maxLod = static_cast<float>(mipLevels);
                // Only enable anisotropic filtering if enabled on the devicec
                samplerCreateInfo.maxAnisotropy = 1.0f;
                samplerCreateInfo.anisotropyEnable = VK_FALSE;
                samplerCreateInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
                texture.sampler = Context::getLogicalDevice().createSamplerUnique(samplerCreateInfo);
                
                texture.imageView = Context::getLogicalDevice().createImageViewUnique(vk::ImageViewCreateInfo{
                    {},
                    texture.image.get(),
                    vk::ImageViewType::eCube,
                    vk::Format::eR8G8B8A8Unorm,
                    {},
                    vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, NUMBER_OF_FACES_FOR_CUBE},
                });
                
                Context::getLogicalDevice().destroyBuffer(stagingBuffer);
                Context::getLogicalDevice().freeMemory(stagingBufferMemory);
            }
        }

    }
}
