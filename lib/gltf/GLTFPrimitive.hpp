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
            } material{{1.0F, 1.0F, 1.0F, 1.0F}, 0.0F, 1.0F};
            
            uint32_t startIndex{};
            uint32_t startVertex{};
            uint32_t indexCount{};
            uint32_t vertexCount{};

            std::vector<vk::UniqueDescriptorSet> descriptorSets{};
        };
    }

#endif /* PvkGLTFPrimitive_hpp */
