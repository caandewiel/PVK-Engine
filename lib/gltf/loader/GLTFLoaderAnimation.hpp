//
// Created by Christian aan de Wiel on 03/05/2021.
//

#ifndef PVK_GLTFLOADERANIMATION_H
#define PVK_GLTFLOADERANIMATION_H

#include <tiny_gltf/tiny_gltf.h>

#include "../GLTFAnimation.hpp"

namespace pvk::gltf::loader::animation {
    std::unique_ptr<Animation> getAnimation(
            const tinygltf::Model &model,
            const tinygltf::Animation &animation,
            const boost::container::flat_map<uint32_t, std::weak_ptr<Node>> &nodeLookup
    );
}  // namespace pvk::gltf::loader::animation

#endif //PVK_GLTFLOADERANIMATION_H
