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

namespace pvk::buffer {
    inline auto findMemoryType(const uint32_t typeFilter,
                               const vk::MemoryPropertyFlags properties) -> uint32_t {
        vk::PhysicalDeviceMemoryProperties memProperties = Context::getPhysicalDevice().getMemoryProperties();

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if (((typeFilter & (1 << i)) != 0U) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
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
                vk::UniqueBuffer &buffer,
                vk::UniqueDeviceMemory &bufferMemory) {
        vk::BufferCreateInfo bufferInfo = {{}, size, usage, vk::SharingMode::eExclusive};

        try {
            buffer = Context::getLogicalDevice().createBufferUnique(bufferInfo);
        } catch (vk::SystemError &error) {
            throw std::runtime_error("Failed to create buffer.");
        }

        vk::MemoryRequirements memRequirements = Context::getLogicalDevice().getBufferMemoryRequirements(buffer.get());
        vk::MemoryAllocateInfo allocInfo = {memRequirements.size,
                                            findMemoryType(memRequirements.memoryTypeBits, properties)};

        try {
            bufferMemory = Context::getLogicalDevice().allocateMemoryUnique(allocInfo);
        } catch (vk::SystemError &error) {
            throw std::runtime_error("Failed to allocate buffer memory.");
        }

        Context::getLogicalDevice().bindBufferMemory(buffer.get(), bufferMemory.get(), 0);
    }

    /**
     Copies the contents from the source buffer to the target buffer.
     */
    void copy(vk::Queue &graphicsQueue,
              vk::UniqueBuffer &srcBuffer,
              vk::UniqueBuffer &dstBuffer,
              vk::DeviceSize size) {
        vk::CommandBufferAllocateInfo allocInfo = {Context::getCommandPool(), vk::CommandBufferLevel::ePrimary, 1};
        vk::BufferCopy copyRegion = {0, 0, size};

        auto commandBuffer = std::move(Context::getLogicalDevice().allocateCommandBuffersUnique(allocInfo)[0]);
        commandBuffer.get().begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        commandBuffer.get().copyBuffer(srcBuffer.get(), dstBuffer.get(), copyRegion);
        commandBuffer.get().end();

        vk::SubmitInfo submitInfo = {{}, {}, {}, 1, &commandBuffer.get()};
        graphicsQueue.submit(submitInfo, nullptr);
        graphicsQueue.waitIdle();
    }

    void copyToImage(
            const vk::CommandBuffer &commandBuffer,
            const vk::Queue &graphicsQueue,
            const vk::UniqueBuffer &buffer,
            const vk::Image &image,
            uint32_t width, uint32_t height, uint32_t numberOfLayers
    ) {
        vk::BufferImageCopy region = {
                0, 0, 0,
                {vk::ImageAspectFlagBits::eColor, 0, 0, numberOfLayers},
                {0, 0, 0},
                {width, height, 1},
        };

        commandBuffer.copyBufferToImage(buffer.get(), image, vk::ImageLayout::eTransferDstOptimal, region);
    }


    void update(const vk::UniqueDeviceMemory &bufferMemory, size_t bufferSize, void *data) {
        void *dataMapped = nullptr;
        auto result = Context::getLogicalDevice().mapMemory(bufferMemory.get(), 0, bufferSize, {}, &dataMapped);

        if (result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to map memory");
        }

        memcpy(dataMapped, data, bufferSize);
        Context::getLogicalDevice().unmapMemory(bufferMemory.get());
    }

//    template<typename T, vk::BufferUsageFlagBits F>
//    auto create(vk::Queue &graphicsQueue,
//                vk::UniqueBuffer &buffer,
//                vk::UniqueDeviceMemory &bufferMemory,
//                std::vector<T> &allItem
//    ) -> void {
//        vk::DeviceSize bufferSize = sizeof(allItem.front()) * allItem.size();
//
//        vk::UniqueBuffer stagingBuffer;
//        vk::UniqueDeviceMemory stagingBufferMemory;
//        pvk::buffer::create(bufferSize,
//                            vk::BufferUsageFlagBits::eTransferSrc,
//                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
//                            stagingBuffer,
//                            stagingBufferMemory);
//        void *data = Context::getLogicalDevice().mapMemory(stagingBufferMemory.get(), 0, bufferSize);
//        memcpy(data, allItem.data(), (size_t) bufferSize);
//
//        pvk::buffer::create(bufferSize,
//                            F,
//                            vk::MemoryPropertyFlagBits::eDeviceLocal,
//                            buffer,
//                            bufferMemory);
//
//        pvk::buffer::copy(graphicsQueue, stagingBuffer, buffer, bufferSize);
//    }

    namespace vertex {
        void create(vk::Queue &graphicsQueue,
                    vk::UniqueBuffer &buffer,
                    vk::UniqueDeviceMemory &bufferMemory,
                    std::vector<Vertex> &vertices) {
            vk::DeviceSize bufferSize = sizeof(vertices.front()) * vertices.size();

            vk::UniqueBuffer stagingBuffer;
            vk::UniqueDeviceMemory stagingBufferMemory;
            pvk::buffer::create(bufferSize,
                                vk::BufferUsageFlagBits::eTransferSrc,
                                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                stagingBuffer,
                                stagingBufferMemory);
            void *data = Context::getLogicalDevice().mapMemory(stagingBufferMemory.get(), 0, bufferSize);
            memcpy(data, vertices.data(), (size_t) bufferSize);

            pvk::buffer::create(bufferSize,
                                vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                                vk::MemoryPropertyFlagBits::eDeviceLocal,
                                buffer,
                                bufferMemory);

            pvk::buffer::copy(graphicsQueue, stagingBuffer, buffer, bufferSize);
        }
    }

    namespace index {
        void create(vk::Queue &graphicsQueue,
                    vk::UniqueBuffer &buffer,
                    vk::UniqueDeviceMemory &bufferMemory,
                    std::vector<uint32_t> &indices) {
            vk::DeviceSize bufferSize = sizeof(indices.front()) * indices.size();

            vk::UniqueBuffer stagingBuffer;
            vk::UniqueDeviceMemory stagingBufferMemory;
            pvk::buffer::create(bufferSize,
                                vk::BufferUsageFlagBits::eTransferSrc,
                                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                stagingBuffer,
                                stagingBufferMemory);
            void *data = Context::getLogicalDevice().mapMemory(stagingBufferMemory.get(), 0, bufferSize);
            memcpy(data, indices.data(), (size_t) bufferSize);

            pvk::buffer::create(bufferSize,
                                vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                                vk::MemoryPropertyFlagBits::eDeviceLocal,
                                buffer,
                                bufferMemory);

            pvk::buffer::copy(graphicsQueue, stagingBuffer, buffer, bufferSize);
        }
    }

    namespace texture {
        constexpr uint8_t NUMBER_OF_PIXEL_PER_COLOR = 4;

        void createEmpty(const vk::Queue &graphicsQueue, pvk::Texture &texture) {
            vk::UniqueBuffer stagingBuffer;
            vk::UniqueDeviceMemory stagingBufferMemory;

            std::array<unsigned char, NUMBER_OF_PIXEL_PER_COLOR> pixels = {0, 0, 0, 0};

            pvk::buffer::create(
                    pixels.size(),
                    vk::BufferUsageFlagBits::eTransferSrc,
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                    stagingBuffer,
                    stagingBufferMemory
            );

            void *data = Context::getLogicalDevice().mapMemory(stagingBufferMemory.get(), 0, pixels.size());
            memcpy(data, &pixels, pixels.size());

            vk::BufferImageCopy bufferCopyRegion;
            bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.imageSubresource.mipLevel = 0;
            bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
            bufferCopyRegion.imageExtent.width = 1;
            bufferCopyRegion.imageExtent.height = 1;

            pvk::image::create(1, 1, 1, 1,
                               vk::SampleCountFlagBits::e1,
                               vk::Format::eR8G8B8A8Unorm,
                               vk::ImageTiling::eOptimal,
                               vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                               vk::MemoryPropertyFlagBits::eDeviceLocal,
                               {},
                               texture.image, texture.imageMemory);

            auto commandBuffers = pvk::util::beginOneTimeCommandBuffer();

            pvk::image::transitionLayout(commandBuffers.front(), graphicsQueue, texture.image.get(),
                                         vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined,
                                         vk::ImageLayout::eTransferDstOptimal, 1, 1);

            pvk::buffer::copyToImage(commandBuffers.front(), graphicsQueue, stagingBuffer, texture.image.get(), 1, 1,
                                     1);

            pvk::image::transitionLayout(commandBuffers.front(), graphicsQueue, texture.image.get(),
                                         vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal,
                                         vk::ImageLayout::eShaderReadOnlyOptimal, 1, 1);

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
            // Only enable anisotropic filtering if enabled on the device
            samplerCreateInfo.maxAnisotropy = 1.0F;
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
        }

        void create(const vk::Queue &graphicsQueue,
                    const tinygltf::Image &gltfImage,
                    pvk::Texture &texture) {
            vk::UniqueBuffer stagingBuffer;
            vk::UniqueDeviceMemory stagingBufferMemory;

            assert(gltfImage.component != 3);

            pvk::buffer::create(
                    gltfImage.image.size(),
                    vk::BufferUsageFlagBits::eTransferSrc,
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                    stagingBuffer,
                    stagingBufferMemory
            );

            void *data = Context::getLogicalDevice().mapMemory(stagingBufferMemory.get(), 0, gltfImage.image.size());
            memcpy(data, gltfImage.image.data(), gltfImage.image.size());

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

            pvk::image::transitionLayout(commandBuffers.front(), graphicsQueue, texture.image.get(),
                                         vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined,
                                         vk::ImageLayout::eTransferDstOptimal, 1, 1);

            pvk::buffer::copyToImage(commandBuffers.front(), graphicsQueue, stagingBuffer, texture.image.get(), width,
                                     height, 1);

            pvk::image::transitionLayout(commandBuffers.front(), graphicsQueue, texture.image.get(),
                                         vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal,
                                         vk::ImageLayout::eShaderReadOnlyOptimal, 1, 1);

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
            samplerCreateInfo.maxAnisotropy = 1.0F;
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
        }

        void create(const vk::Queue &graphicsQueue,
                    const gli::texture_cube &textureCube,
                    pvk::Texture &texture) {
            vk::UniqueBuffer stagingBuffer;
            vk::UniqueDeviceMemory stagingBufferMemory;
            pvk::buffer::create(textureCube.size(),
                                vk::BufferUsageFlagBits::eTransferSrc,
                                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                stagingBuffer,
                                stagingBufferMemory);

            void *data = Context::getLogicalDevice().mapMemory(stagingBufferMemory.get(), 0, textureCube.size());
            memcpy(data, textureCube.data(), textureCube.size());

            auto width = static_cast<uint32_t>(textureCube.extent().x);
            auto height = static_cast<uint32_t>(textureCube.extent().y);
            auto mipLevels = static_cast<uint32_t>(textureCube.levels());
            auto numberOfLayers = static_cast<uint32_t>(textureCube.layers());

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

            pvk::image::transitionLayout(commandBuffers.front(), graphicsQueue, texture.image.get(),
                                         vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined,
                                         vk::ImageLayout::eTransferDstOptimal, mipLevels, NUMBER_OF_FACES_FOR_CUBE);

            pvk::buffer::copyToImage(commandBuffers.front(), graphicsQueue, stagingBuffer, texture.image.get(), width,
                                     height, NUMBER_OF_FACES_FOR_CUBE);

            pvk::image::transitionLayout(commandBuffers.front(), graphicsQueue, texture.image.get(),
                                         vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal,
                                         vk::ImageLayout::eShaderReadOnlyOptimal, mipLevels, NUMBER_OF_FACES_FOR_CUBE);

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
            samplerCreateInfo.maxAnisotropy = 1.0F;
            samplerCreateInfo.anisotropyEnable = VK_FALSE;
            samplerCreateInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
            texture.sampler = Context::getLogicalDevice().createSamplerUnique(samplerCreateInfo);

            texture.imageView = Context::getLogicalDevice().createImageViewUnique(vk::ImageViewCreateInfo{
                    {},
                    texture.image.get(),
                    vk::ImageViewType::eCube,
                    vk::Format::eR8G8B8A8Unorm,
                    {},
                    vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0,
                                              NUMBER_OF_FACES_FOR_CUBE},
            });
        }
    }

}
