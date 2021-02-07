#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
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

layout (location = 0) out vec3 outUVW;

void main() 
{
	outUVW = inPosition;
	gl_Position = ubo.proj * mat4(mat3(ubo.view)) * model.model * vec4(inPosition.xyz, 1.0);
}
