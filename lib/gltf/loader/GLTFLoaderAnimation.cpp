//
// Created by Christian aan de Wiel on 03/05/2021.
//

#include <span>
#include <vector>
#include <boost/container/flat_map.hpp>

#include "GLTFLoaderAnimation.hpp"

namespace {
    std::vector<float> loadAnimationInputs(
            const tinygltf::Model &model,
            const tinygltf::AnimationSampler &sampler
    ) {
        const auto &accessor = model.accessors[sampler.input];
        const auto &bufferView = model.bufferViews[accessor.bufferView];
        const auto &buffer = model.buffers[bufferView.buffer];

        const void *bufferPointer = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
        const auto timeValueBuffer = std::span<const float>(
                static_cast<const float *>(bufferPointer),
                bufferView.byteLength
        );

        std::vector<float> inputs;

        inputs.reserve(accessor.count);

        for (size_t i = 0; i < accessor.count; i++) {
            inputs.emplace_back(timeValueBuffer[i]);
        }

        return inputs;
    }

    std::vector<glm::vec4> loadAnimationOutputs(
            const tinygltf::Model &model,
            const tinygltf::AnimationSampler &sampler
    ) {
        const auto &accessor = model.accessors[sampler.output];
        const auto &bufferView = model.bufferViews[accessor.bufferView];
        const auto &buffer = model.buffers[bufferView.buffer];
        const void *bufferPointer = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

        std::vector<glm::vec4> outputs;
        outputs.reserve(accessor.count);

        switch (accessor.type) {
            case TINYGLTF_TYPE_VEC3: {
                const auto outputsSpan = std::span<const glm::vec3>(
                        static_cast<const glm::vec3 *>(bufferPointer),
                        bufferView.byteLength
                );

                for (const auto &output : outputsSpan) {
                    outputs.emplace_back(glm::vec4(output, 0.0F));
                }

                break;
            }
            case TINYGLTF_TYPE_VEC4: {
                const auto outputsSpan = std::span<const glm::vec4>(
                        static_cast<const glm::vec4 *>(bufferPointer),
                        bufferView.byteLength
                );

                for (const auto &output : outputsSpan) {
                    outputs.emplace_back(output);
                }

                break;
            }
            case TINYGLTF_TYPE_SCALAR: {
                break;
            }
            default:
                throw std::runtime_error("Unsupported animation output type");
        }

        return outputs;
    }

    pvk::gltf::Sampler::InterpolationType getInterpolationType(
            const tinygltf::AnimationSampler &sampler
    ) {
        if (sampler.interpolation == "LINEAR") {
            return pvk::gltf::Sampler::InterpolationType::LINEAR;
        }

        if (sampler.interpolation == "STEP") {
            return pvk::gltf::Sampler::InterpolationType::STEP;
        }

        if (sampler.interpolation == "CUBICSPLINE") {
            return pvk::gltf::Sampler::InterpolationType::CUBICSPLINE;
        }

        throw std::runtime_error("Unsupported animation interpolation type");
    }

    pvk::gltf::Sampler getAnimationSampler(
            const tinygltf::Model &model,
            const tinygltf::AnimationSampler &sampler
    ) {
        pvk::gltf::Sampler _sampler;

        _sampler.inputs = loadAnimationInputs(model, sampler);
        _sampler.outputs = loadAnimationOutputs(model, sampler);
        _sampler.interpolationType = getInterpolationType(sampler);

        return _sampler;
    }

    pvk::gltf::Channel getAnimationChannel(
            const tinygltf::AnimationChannel &channel,
            const boost::container::flat_map<uint32_t, std::weak_ptr<pvk::gltf::Node>> &nodeLookup
    ) {
        pvk::gltf::Channel _channel;

        if (channel.target_path == "rotation") {
            _channel.pathType = pvk::gltf::Channel::PathType::ROTATION;
        } else if (channel.target_path == "translation") {
            _channel.pathType = pvk::gltf::Channel::PathType::TRANSLATION;
        } else if (channel.target_path == "scale") {
            _channel.pathType = pvk::gltf::Channel::PathType::SCALE;
        } else {
            // @TODO: Implement weights later.
        }

        _channel.samplerIndex = channel.sampler;
        _channel.node = nodeLookup.at(channel.target_node);

        return _channel;
    }
}  // namespace

namespace pvk::gltf::loader::animation {
    std::unique_ptr<Animation> getAnimation(
            const tinygltf::Model &model,
            const tinygltf::Animation &animation,
            const boost::container::flat_map<uint32_t, std::weak_ptr<Node>> &nodeLookup
    ) {
        auto _animation = std::make_unique<gltf::Animation>();
        _animation->samplers.reserve(animation.samplers.size());
        _animation->channels.reserve(animation.channels.size());

        for (const auto &sampler : animation.samplers) {
            _animation->samplers.emplace_back(getAnimationSampler(model, sampler));
        }

        for (const auto &channel : animation.channels) {
            _animation->channels.emplace_back(getAnimationChannel(channel, nodeLookup));
        }

        _animation->currentTime = 0.0F;
        _animation->startTime = 0.0F;
        _animation->endTime = -1.0F;

        for (auto &sampler : _animation->samplers) {
            for (auto &input : sampler.inputs) {
                if (_animation->endTime == -1.0F || input > _animation->endTime) {
                    _animation->endTime = input;
                }
            }
        }

        return _animation;
    }
}  // namespace pvk::gltf::loader::animation
