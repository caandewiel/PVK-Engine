//
//  GLTFAnimation.hpp
//  VulkanHpp
//
//  Created by Christian aan de Wiel on 29/01/2021.
//

#ifndef GLTFAnimation_hpp
#define GLTFAnimation_hpp

#include <cstdio>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "GLTFNode.hpp"

namespace pvk::gltf {
    struct Sampler {
        enum InterpolationType {
            LINEAR, STEP, CUBICSPLINE
        } interpolationType;
        std::vector<float> inputs;
        std::vector<glm::vec4> outputs;
    };

    struct Channel {
        enum PathType {
            TRANSLATION, ROTATION, SCALE
        } pathType;
        std::shared_ptr<Node> node;
        uint32_t samplerIndex;
    };

    struct Animation {
        float currentTime;
        float startTime;
        float endTime;
        std::vector<Sampler> samplers;
        std::vector<Channel> channels;

        void update(float time) {
            this->currentTime = this->currentTime + time;

            if (this->currentTime > endTime) {
                this->currentTime -= endTime;
            }

            for (auto &channel : channels) {
                auto &sampler = samplers[channel.samplerIndex];

                for (size_t i = 0; i < sampler.inputs.size() - 1; i++) {
                    if (currentTime >= sampler.inputs[i] && time <= sampler.inputs[i + 1]) {
                        float delta = (currentTime - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);

                        switch (channel.pathType) {
                            case Channel::TRANSLATION: {
                                glm::vec4 translation = glm::mix(sampler.outputs[i], sampler.outputs[i + 1], delta);
                                channel.node->translation = glm::vec3(translation);
                                break;
                            }
                            case Channel::ROTATION: {
                                glm::quat rotationSource{
                                        sampler.outputs[i].w,
                                        sampler.outputs[i].x,
                                        sampler.outputs[i].y,
                                        sampler.outputs[i].z,
                                };

                                glm::quat rotationTarget{
                                        sampler.outputs[i + 1].w,
                                        sampler.outputs[i + 1].x,
                                        sampler.outputs[i + 1].y,
                                        sampler.outputs[i + 1].z,
                                };

                                channel.node->rotation = glm::mat4(
                                        glm::normalize(glm::slerp(rotationSource, rotationTarget, delta)));
                                break;
                            }
                            case Channel::SCALE: {
                                glm::vec4 scale = glm::mix(sampler.outputs[i], sampler.outputs[i + 1], delta);
                                channel.node->scale = glm::vec3(scale);
                                break;
                            }
                        }

                        auto translationMatrix = glm::translate(glm::mat4(1.0f), channel.node->translation);
                        auto rotationMatrix = glm::mat4(channel.node->rotation);
                        auto scaleMatrix = glm::scale(glm::mat4(1.0f), channel.node->scale);

                        channel.node->matrix = translationMatrix * rotationMatrix * scaleMatrix * glm::mat4(1.0f);
                    }
                }
            }
        }
    };
}

#endif /* GLTFAnimation_hpp */
