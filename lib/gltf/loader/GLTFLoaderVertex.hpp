//
// Created by Christian aan de Wiel on 03/05/2021.
//

#ifndef PVK_GLTFLOADERVERTEX_HPP
#define PVK_GLTFLOADERVERTEX_HPP

#include <tiny_gltf/tiny_gltf.h>
#include <glm/glm.hpp>

namespace {
    template<typename T>
    constexpr uint8_t getComponentType() {
        static_assert(T::length() == 2 || T::length() == 3 || T::length() == 4, "Unsupported component type");

        if constexpr (T::length() == 2) {
            return TINYGLTF_TYPE_VEC2;
        } else if constexpr (T::length() == 3) {
            return TINYGLTF_TYPE_VEC3;
        } else if constexpr (T::length() == 4) {
            return TINYGLTF_TYPE_VEC4;
        }
    }

    template<typename T>
    T loadBuffer(
            const std::shared_ptr<tinygltf::Model> &model,
            const tinygltf::Primitive &primitive,
            const std::string &field,
            uint32_t index
    ) {
        const auto &accessor = model->accessors[primitive.attributes.find(field)->second];
        const auto &bufferView = model->bufferViews[accessor.bufferView];

        // Determine byte stride
        uint32_t byteStride = 0;
        if (accessor.ByteStride(bufferView) > 1) {
            byteStride = accessor.ByteStride(bufferView);
        } else {
            byteStride = tinygltf::GetComponentSizeInBytes(getComponentType<T>());
        }

        auto dataOffset = accessor.byteOffset + bufferView.byteOffset;

        if (byteStride == sizeof(T)) {
            const auto *buffer = reinterpret_cast<const typename T::value_type *>(
                    &(model->buffers[bufferView.buffer].data[dataOffset])
            );

            T result{};
            std::memcpy(&result, buffer + index * T::length(), sizeof(T)); // NOLINT

            return result;
        }

        throw std::runtime_error("glTF model contains invalid byte stride.");
    }
}  // namespace

namespace pvk::gltf::loader::vertex {
    inline glm::vec3 getVertexPositionByPrimitive(
            const std::shared_ptr<tinygltf::Model> &model,
            const tinygltf::Primitive &primitive,
            const uint32_t index
    ) {
        return loadBuffer<glm::vec3>(model, primitive, FIELD_VERTEX_POSITION, index);
    }

    inline glm::vec3 getVertexNormalByPrimitive(
            const std::shared_ptr<tinygltf::Model> &model,
            const tinygltf::Primitive &primitive,
            const uint32_t index
    ) {
        if (primitive.attributes.find(FIELD_VERTEX_NORMAL) == primitive.attributes.end()) {
            // Skip loading normals if the model does not contain any.
            return glm::vec3(0.0F);
        }

        return loadBuffer<glm::vec3>(model, primitive, FIELD_VERTEX_NORMAL, index);
    }

    inline glm::vec3 getVertexColorByPrimitive(
            const std::shared_ptr<tinygltf::Model> &model,
            const tinygltf::Primitive &primitive,
            const uint32_t index
    ) {
        if (primitive.attributes.find(FIELD_VERTEX_COLOR_0) == primitive.attributes.end()) {
            // Make the default color of a mesh white, so it's easily visible.
            return glm::vec3(1.0F);
        }

        switch (model->accessors[primitive.attributes.find(FIELD_VERTEX_COLOR_0)->second].type) {
            case TINYGLTF_TYPE_VEC3: {
                return loadBuffer<glm::vec3>(model, primitive, FIELD_VERTEX_COLOR_0, index);
            }
            case TINYGLTF_TYPE_VEC4: {
                return loadBuffer<glm::vec4>(model, primitive, FIELD_VERTEX_COLOR_0, index);
            }
            default: {
                throw std::runtime_error("Invalid vertex color type.");
            }
        }
    }

    inline glm::vec2 getVertexUV0ByPrimitive(
            const std::shared_ptr<tinygltf::Model> &model,
            const tinygltf::Primitive &primitive,
            const uint32_t index
    ) {
        if (primitive.attributes.find(FIELD_VERTEX_TEXCOORD_0) == primitive.attributes.end()) {
            return glm::vec2(0.0F);
        }

        return loadBuffer<glm::vec2>(model, primitive, FIELD_VERTEX_TEXCOORD_0, index);
    }

    inline glm::vec2 getVertexUV1ByPrimitive(
            const std::shared_ptr<tinygltf::Model> &model,
            const tinygltf::Primitive &primitive,
            const uint32_t index
    ) {
        if (primitive.attributes.find(FIELD_VERTEX_TEXCOORD_1) == primitive.attributes.end()) {
            return glm::vec2(0.0F);
        }

        return loadBuffer<glm::vec2>(model, primitive, FIELD_VERTEX_TEXCOORD_1, index);
    }

    inline glm::ivec4 getVertexJointByPrimitive(
            const std::shared_ptr<tinygltf::Model> &model,
            const tinygltf::Primitive &primitive,
            const uint32_t index
    ) {
        if (primitive.attributes.find(FIELD_VERTEX_JOINTS_0) == primitive.attributes.end()) {
            return glm::ivec4(0);
        }

        const auto &jointAccessor = model->accessors[primitive.attributes.find(FIELD_VERTEX_JOINTS_0)->second];
        const auto &jointView = model->bufferViews[jointAccessor.bufferView];

        // Determine byte stride
        uint32_t byteStride = 0;
        if (jointAccessor.ByteStride(jointView) > -1) {
            byteStride = jointAccessor.ByteStride(jointView);
        } else {
            byteStride = tinygltf::GetComponentSizeInBytes(TINYGLTF_TYPE_VEC4);
        }

        using uint8_vec4 = glm::vec<4, uint8_t, glm::defaultp>;
        using uint16_vec4 = glm::vec<4, uint16_t, glm::defaultp>;

        switch (byteStride) {
            case sizeof(uint8_vec4): {
                return loadBuffer<uint8_vec4>(model, primitive, FIELD_VERTEX_JOINTS_0, index);
            }
            case sizeof(uint16_vec4): {
                return loadBuffer<uint16_vec4>(model, primitive, FIELD_VERTEX_JOINTS_0, index);
            }
            default: {
                throw std::runtime_error("glTF model contains invalid byte stride for vertex joints.");
            }
        }
    }

    inline glm::vec4 getVertexWeightByPrimitive (
            const std::shared_ptr<tinygltf::Model> &model,
            const tinygltf::Primitive &primitive,
            const uint32_t index
    ) {
        if (primitive.attributes.find(FIELD_VERTEX_WEIGHTS_0) == primitive.attributes.end()) {
            return glm::vec4(0.0F);
        }

        return loadBuffer<glm::vec4>(model, primitive, FIELD_VERTEX_WEIGHTS_0, index);
    }
}  // namespace pvk::gltf::loader::vertex

#endif //PVK_GLTFLOADERVERTEX_HPP
