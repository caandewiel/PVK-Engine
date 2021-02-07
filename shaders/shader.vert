#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 local;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 outNormal;

void main() {
    outNormal = inNormal;
    vec4 localPosition = ubo.model * ubo.local * vec4(inPosition, 1.0);
    outNormal = normalize(transpose(inverse(mat3(ubo.model * ubo.local))) * inNormal);
    gl_Position = ubo.proj * ubo.view * vec4(localPosition.xyz, 1.0);
}
