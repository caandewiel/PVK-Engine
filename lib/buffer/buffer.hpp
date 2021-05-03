//
//  buffer.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 16/01/2021.
//

#ifndef buffer_hpp
#define buffer_hpp

#define NUMBER_OF_FACES_FOR_CUBE 6

#include <utility>
#include <vulkan/vulkan.hpp>
#include "proxy/gli.h"
#include "proxy/tiny_gltf.h"

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
        void create(const unsigned long long int size,
                    const vk::BufferUsageFlags usage,
                    const vk::MemoryPropertyFlags properties,
                    vk::UniqueBuffer &buffer,
                    vk::UniqueDeviceMemory &bufferMemory);
        
        void copy(vk::Queue &graphicsQueue,
                  vk::UniqueBuffer &srcBuffer,
                  vk::UniqueBuffer &dstBuffer,
                  vk::DeviceSize size);
        
        void copyToImage(const vk::CommandBuffer &commandBuffer, const vk::Queue &graphicsQueue,
                         const vk::UniqueBuffer &buffer,
                         const vk::Image &image, uint32_t width, uint32_t height, uint32_t numberOfLayers);
        
        void update(const vk::UniqueDeviceMemory &bufferMemory,
                    size_t bufferSize,
                    void* data);

//        template<typename T, vk::BufferUsageFlagBits F>
//        auto create(vk::Queue &graphicsQueue,
//                    vk::UniqueBuffer &buffer,
//                    vk::UniqueDeviceMemory &bufferMemory,
//                    std::vector<T> &allItem
//        ) -> void;

        namespace vertex {
            void create(vk::Queue &graphicsQueue,
                        vk::UniqueBuffer &buffer,
                        vk::UniqueDeviceMemory &bufferMemory,
                        std::vector<Vertex> &vertices);
        }
        
        namespace index {
            void create(vk::Queue &graphicsQueue,
                        vk::UniqueBuffer &buffer,
                        vk::UniqueDeviceMemory &bufferMemory,
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
