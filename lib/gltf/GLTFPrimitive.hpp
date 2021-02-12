//
//  PvkGLTFPrimitive.hpp
//  PVK
//
//  Created by Christian aan de Wiel on 10/01/2021.
//

#ifndef PvkGLTFPrimitive_hpp
#define PvkGLTFPrimitive_hpp

#include <cstdio>
#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <map>

namespace pvk::gltf {
        class Primitive {
        public:
            Primitive();
            ~Primitive();
            
            struct Material {
                glm::vec4 baseColorFactor;
                float metallicFactor;
                float roughnessFactor;
            } material{{1.0f, 1.0f, 1.0f, 1.0f}, 0.0f, 1.0f};
            
            uint32_t startIndex{};
            uint32_t startVertex{};
            uint32_t indexCount{};
            uint32_t vertexCount{};

            std::vector<vk::UniqueDescriptorSet> descriptorSets{};
            std::map<uint32_t, std::vector<vk::DescriptorBufferInfo>> descriptorBuffersInfo{};
            std::map<uint32_t, std::vector<vk::Buffer>> uniformBuffers{};
            std::map<uint32_t, std::vector<vk::DeviceMemory>> uniformBuffersMemory{};
        };
    }

#endif /* PvkGLTFPrimitive_hpp */
