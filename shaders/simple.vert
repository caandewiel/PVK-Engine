#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

void main() {
    vec4 localPosition = vec4(inPosition, 1.0);
    gl_Position = vec4(localPosition.xyz, 1.0);
}