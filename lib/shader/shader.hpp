//
//  shader.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 21/01/2021.
//

#ifndef shader_hpp
#define shader_hpp

#include <cstdio>
#include <string>
#include <map>
#include <vulkan/vulkan.hpp>

#include "../buffer/buffer.hpp"

namespace pvk {
    enum ShaderStage {VERTEX_ONLY, FRAGMENT_ONLY, VERTEX_AND_FRAGMENT};
    enum DescriptorType {UNIFORM_BUFFER, COMBINED_IMAGE_SAMPLER};
    
    struct DescriptorSetLayoutBinding {
        uint32_t bindingIndex;
        DescriptorType type;
        ShaderStage shaderStage;
        size_t size;
    };
    
    class Shader {
    public:
        Shader(std::string fileVertexShader, std::string fileFragmentShader, std::vector<DescriptorSetLayoutBinding> bindings);
        void setBindingBuffer(uint32_t bindingIndex, ShaderStage shaderStage, size_t size);
        std::vector<vk::DescriptorSetLayoutBinding> getDescriptorSetLayoutBindings();
        vk::ShaderModule getVertexShaderModule();
        vk::ShaderModule getFragmentShaderModule();
        size_t getSizeForBinding(uint32_t bindingIndex);
        
    private:
        vk::ShaderModule loadShader(std::string filePath, vk::ShaderStageFlagBits stage);
        
        vk::ShaderModule vertexShaderModule {};
        vk::ShaderModule fragmentShaderModule {};
        
        std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {};
        std::vector<vk::DeviceSize> descriptorSetLayoutBindingSizes = {};
        
        static std::map<ShaderStage, vk::ShaderStageFlags> shaderStageMapping;
        static std::map<DescriptorType, vk::DescriptorType> descriptorTypeMapping;
    };
}

#endif /* shader_hpp */
