//
//  buffer.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 16/01/2021.
//

#ifndef buffer_hpp
#define buffer_hpp

#define NUMBER_OF_FACES_FOR_CUBE 6

#include <cstdio>
#include <utility>
#include <vulkan/vulkan.hpp>
#include <gli.hpp>

#include "tiny_gltf.h"

#include "../mesh/vertex.hpp"
#include "../image/image.hpp"
#include "../texture/texture.hpp"
#include "../context/context.hpp"

namespace pvk {
    class Buffer {
    public:
        Buffer(std::vector<vk::Buffer> buffer, std::vector<vk::DeviceMemory> bufferMemory): buffer(std::move(buffer)), bufferMemory(std::move(bufferMemory)) {};

    private:
        std::vector<vk::Buffer> buffer;
        std::vector<vk::DeviceMemory> bufferMemory;
    };
    
    namespace buffer {
        void create(vk::DeviceSize size,
                    vk::BufferUsageFlags usage,
                    vk::MemoryPropertyFlags properties,
                    vk::Buffer &buffer,
                    vk::DeviceMemory &bufferMemory);
        
        void copy(vk::Queue &graphicsQueue,
                  vk::Buffer &srcBuffer,
                  vk::Buffer &dstBuffer,
                  vk::DeviceSize size);
        
        void copyToImage(const vk::CommandBuffer &commandBuffer,
                         const vk::Queue &graphicsQueue,
                         const vk::Buffer &buffer,
                         const vk::Image &image,
                         uint32_t width, uint32_t height);
        
        void update(vk::DeviceMemory bufferMemory,
                    size_t bufferSize,
                    void* data);
        
        namespace vertex {
            void create(vk::Queue &graphicsQueue,
                        vk::Buffer &buffer,
                        vk::DeviceMemory &bufferMemory,
                        std::vector<Vertex> &vertices);
        }
        
        namespace index {
            void create(vk::Queue &graphicsQueue,
                        vk::Buffer &buffer,
                        vk::DeviceMemory &bufferMemory,
                        std::vector<uint32_t> &indices);
        }
        
        namespace texture {
            void create(const vk::Queue &graphicsQueue,
                        const gli::texture_cube &textureCube,
                        pvk::Texture &texture);

            void create(const vk::Queue &graphicsQueue,
                        const tinygltf::Image &gltfImage,
                        pvk::Texture &texture);

            void createEmpty(const vk::Queue &graphicsQueue, pvk::Texture &texture);
        }
    }
}

#endif /* buffer_hpp */
