//
//  shader.cpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 21/01/2021.
//

#include "shader.hpp"

namespace pvk {
    std::map<ShaderStage, vk::ShaderStageFlags> Shader::shaderStageMapping = {
        {ShaderStage::VERTEX_ONLY, vk::ShaderStageFlagBits::eVertex},
        {ShaderStage::FRAGMENT_ONLY, vk::ShaderStageFlagBits::eFragment},
        {ShaderStage::VERTEX_AND_FRAGMENT, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
    };
    
    std::map<DescriptorType, vk::DescriptorType> Shader::descriptorTypeMapping = {
        {DescriptorType::UNIFORM_BUFFER, vk::DescriptorType::eUniformBuffer},
        {DescriptorType::COMBINED_IMAGE_SAMPLER, vk::DescriptorType::eCombinedImageSampler},
    };
    
    Shader::Shader(std::string fileVertexShader, std::string fileFragmentShader, std::vector<DescriptorSetLayoutBinding> bindings) {
        this->vertexShaderModule = this->loadShader(fileVertexShader, vk::ShaderStageFlagBits::eVertex);
        this->fragmentShaderModule = this->loadShader(fileFragmentShader, vk::ShaderStageFlagBits::eFragment);
        
        for (auto &binding : bindings) {
            this->descriptorSetLayoutBindings.push_back({binding.bindingIndex, Shader::descriptorTypeMapping[binding.type], 1, Shader::shaderStageMapping[binding.shaderStage]});
            this->descriptorSetLayoutBindingSizes.push_back(binding.size);
        }
    }
    
    void Shader::setBindingBuffer(uint32_t bindingIndex, ShaderStage shaderStage, size_t size) {
        if (bindingIndex >= (uint32_t) this->descriptorSetLayoutBindings.size()) {
            throw std::runtime_error("Invalid binding index");
        }
        
        this->descriptorSetLayoutBindings[bindingIndex] = {bindingIndex, vk::DescriptorType::eUniformBuffer, 1, Shader::shaderStageMapping[shaderStage]};
        this->descriptorSetLayoutBindingSizes[bindingIndex] = size;
    }
    
    vk::ShaderModule Shader::loadShader(std::string filePath, vk::ShaderStageFlagBits stage) {
        auto code = pvk::util::readFile(filePath);
        
        try {
            return pvk::Context::getLogicalDevice().createShaderModule({
                vk::ShaderModuleCreateFlags(),
                code.size(),
                reinterpret_cast<const uint32_t*>(code.data())
            });
        } catch (vk::SystemError err) {
            throw std::runtime_error("Failed to load shader.");
        }
    }
    
    std::vector<vk::DescriptorSetLayoutBinding> Shader::getDescriptorSetLayoutBindings() {
        return this->descriptorSetLayoutBindings;
    }
    
    vk::ShaderModule Shader::getVertexShaderModule() {
        return this->vertexShaderModule;
    }
    
    vk::ShaderModule Shader::getFragmentShaderModule() {
        return this->fragmentShaderModule;
    }
    
    size_t Shader::getSizeForBinding(uint32_t bindingIndex) {
        if (bindingIndex >= (uint32_t) this->descriptorSetLayoutBindingSizes.size()) {
            throw std::runtime_error("Invalid binding index");
        }
        
        return this->descriptorSetLayoutBindingSizes[bindingIndex];
    }
}
