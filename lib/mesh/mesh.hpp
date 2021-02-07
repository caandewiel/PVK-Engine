//
//  Mesh.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 17/01/2021.
//

#ifndef Mesh_hpp
#define Mesh_hpp

#include <stdio.h>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "vertex.hpp"

namespace pvk {
    class Mesh
    {
    public:
        Mesh();
        ~Mesh();

        uint32_t startingIndex;
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
    };
}

#endif /* Mesh_hpp */
