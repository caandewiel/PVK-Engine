//
//  object.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 21/01/2021.
//

#ifndef object_hpp
#define object_hpp

#include <cstdio>
#include <string>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "../gltf/GLTFLoader.hpp"

namespace pvk {
    class Object {
    public:
        static Object *createFromGLTF(vk::Queue &graphicsQueue, const std::string &filename);

        ~Object();

        [[nodiscard]] std::vector<gltf::Node *> getNodes() const;

        [[nodiscard]] std::vector<gltf::Animation *> getAnimations() const;

        void updateUniformBuffer(uint32_t bindingIndex, size_t size, void *data) const;

        void updateUniformBufferPerNode(uint32_t bindingIndex,
                                        const std::function<void(pvk::gltf::Object *object,
                                                                 pvk::gltf::Node *node,
                                                                 vk::DeviceMemory &memory)> &function) const;
        // @TODO: Make this private
        gltf::Object *gltfObject;

    private:
        Object();
    };
}

#endif /* object_hpp */
