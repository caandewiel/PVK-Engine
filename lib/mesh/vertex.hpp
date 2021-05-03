//
//  vertex.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 17/01/2021.
//

#ifndef vertex_hpp
#define vertex_hpp


#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace pvk {
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec3 normal;
        glm::vec2 UV0;
        glm::vec2 UV1;
        glm::ivec4 joint;
        glm::vec4 weight;

        static std::vector<vk::VertexInputBindingDescription> getBindingDescription() {
            vk::VertexInputBindingDescription bindingDescription {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
            
            std::vector<vk::VertexInputBindingDescription> bindingDescriptions = {
                bindingDescription,
            };

            return bindingDescriptions;
        }

        static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions() {
            vk::VertexInputAttributeDescription position_   {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)};
            vk::VertexInputAttributeDescription color_      {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)};
            vk::VertexInputAttributeDescription normal_     {2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)};
            vk::VertexInputAttributeDescription UV0_        {3, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, UV0)};
            vk::VertexInputAttributeDescription UV1_        {4, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, UV1)};
            vk::VertexInputAttributeDescription joints0_    {5, 0, vk::Format::eR32G32B32A32Sint, offsetof(Vertex, joint)};
            vk::VertexInputAttributeDescription weights0_   {6, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, weight)};

            std::vector<vk::VertexInputAttributeDescription> attributeDescriptions = {position_, color_, normal_, UV0_, UV1_, joints0_, weights0_};

            return attributeDescriptions;
        }
    };
}

#endif /* vertex_hpp */
