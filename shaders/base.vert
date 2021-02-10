#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 cameraPosition;
    vec3 lightPosition;
} ubo;

layout(binding = 1) uniform BufferObject {
    mat4 model;
    mat4 local;
    mat4 inverseBindMatrices[256];
    float jointCount;
} model;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV0;
layout(location = 4) in vec2 inUV1;
layout(location = 5) in ivec4 inJoint0;
layout(location = 6) in vec4 inWeight0;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV0;
layout(location = 3) out vec2 outUV1;
layout(location = 4) out vec3 outLightPosition;
layout(location = 5) out vec3 outCameraPosition;

void main() {
    vec4 localPosition;

    if (model.jointCount > 0.0) {
        // Mesh is skinned
        mat4 skinMat =
        inWeight0.x * model.inverseBindMatrices[int(inJoint0.x)] +
        inWeight0.y * model.inverseBindMatrices[int(inJoint0.y)] +
        inWeight0.z * model.inverseBindMatrices[int(inJoint0.z)] +
        inWeight0.w * model.inverseBindMatrices[int(inJoint0.w)];

        localPosition = model.model * model.local * skinMat * vec4(inPosition, 1.0);
        outNormal = normalize(mat3(model.model * model.local * skinMat) * inNormal);
    } else {
        localPosition = model.model * model.local * vec4(inPosition, 1.0);
        outNormal = normalize(transpose(inverse(mat3(model.model * model.local))) * inNormal);
    }
    outPosition = vec3(localPosition);
    
    outLightPosition = ubo.lightPosition;
    
    outCameraPosition = ubo.cameraPosition;

    outUV0 = inUV0;

    outUV1 = inUV1;

    gl_Position = ubo.proj * ubo.view * vec4(localPosition.xyz, 1.0);
}
