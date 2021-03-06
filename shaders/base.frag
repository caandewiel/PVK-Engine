#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform Material {
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
} material;

layout(set = 1, binding = 1) uniform sampler2D baseColorSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampler;
layout(set = 1, binding = 3) uniform sampler2D metallicRoughnessSampler;
layout(set = 1, binding = 4) uniform sampler2D occlusionSampler;
layout(set = 1, binding = 5) uniform sampler2D emissiveSampler;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV0;
layout(location = 3) in vec2 inUV1;
layout(location = 4) in vec3 inLightPosition;
layout(location = 5) in vec3 inCameraPosition;

layout(location = 0) out vec4 outColor;

struct MaterialBooleans {
    bool hasBaseColorTexture;
    bool hasNormalTexture;
    bool hasMetallicRoughnessTexture;
    bool hasEmissiveTexture;
    bool hasOcclusionTexture;
} materialBooleans;

const float PI = 3.14159265359;
#define ALBEDO pow(texture(baseColorSampler, inUV0).rgb, vec3(2.2))

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return (alpha2)/(PI * denom*denom);
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float GL = dotNL / (dotNL * (1.0 - k) + k);
    float GV = dotNV / (dotNV * (1.0 - k) + k);
    return GL * GV;
}

// Fresnel function ----------------------------------------------------
vec3 F_Schlick(float cosTheta, float metallic)
{
    vec3 F0 = mix(vec3(0.04), texture(baseColorSampler, inUV0).xyz, metallic); // * material.specular
//    vec3 F0 = mix(vec3(0.04), material.baseColorFactor.xyz, metallic); // * material.specular
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
    return F;
}

vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// Specular BRDF composition --------------------------------------------

vec3 BRDF(vec3 L, vec3 V, vec3 N, float metallic, float roughness)
{
    // Precalculate vectors and dot products
    vec3 H = normalize (V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotLH = clamp(dot(L, H), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);

    // Light color fixed
    vec3 lightColor = vec3(1.0);

    vec3 color = vec3(0.0);

    if (dotNL > 0.0)
    {
        float rroughness = max(0.05, roughness);
        // D = Normal distribution (Distribution of the microfacets)
        float D = D_GGX(dotNH, roughness);
        // G = Geometric shadowing term (Microfacets shadowing)
        float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
        // F = Fresnel factor (Reflectance depending on angle of incidence)
        vec3 F = F_Schlick(dotNV, metallic);

        vec3 spec = D * F * G / (4.0 * dotNL * dotNV);

        color += spec * dotNL * lightColor;
    }

    return color;
}

void main() {
    materialBooleans.hasBaseColorTexture = textureSize(baseColorSampler, 0).x > 1;
    materialBooleans.hasNormalTexture = textureSize(normalSampler, 0).x > 1;
    materialBooleans.hasMetallicRoughnessTexture = textureSize(metallicRoughnessSampler, 0).x > 1;
    materialBooleans.hasEmissiveTexture = textureSize(emissiveSampler, 0).x > 1;
    materialBooleans.hasOcclusionTexture = textureSize(occlusionSampler, 0).x > 1;

    vec3 N;

    if (materialBooleans.hasNormalTexture) {
        N = normalize(inNormal * texture(normalSampler, inUV0).xyz);
    } else {
        N = normalize(inNormal);
    }

    vec3 V = normalize(inCameraPosition - inPosition);

    float roughness;
    float metallicness;

    if (materialBooleans.hasMetallicRoughnessTexture) {
        roughness = texture(metallicRoughnessSampler, inUV0).g;
        metallicness = texture(metallicRoughnessSampler, inUV0).b;
    } else {
        roughness = material.roughnessFactor;
        metallicness = material.metallicFactor;
    }

    vec3 Lo = vec3(0.0);
    vec3 L = normalize(inLightPosition - inPosition);
    Lo += BRDF(L, V, N, metallicness, roughness);

    vec3 F0 = vec3(0.04);

    if (materialBooleans.hasBaseColorTexture) {
        F0 = mix(F0, ALBEDO, metallicness);
    } else {
        F0 = mix(F0, vec3(material.baseColorFactor), metallicness);
    }

    vec3 F = F_SchlickR(max(dot(N, V), 0.0), F0, roughness);

    // Ambient part
    vec3 kD = 1.0 - F;
    kD *= 1.0 - metallicness;

    vec3 ambient;

    if (materialBooleans.hasBaseColorTexture) {
        ambient = kD * ALBEDO * texture(occlusionSampler, inUV0).rrr;
    } else {
        ambient = kD * vec3(material.baseColorFactor);
    }

    vec3 color;
    if (materialBooleans.hasEmissiveTexture) {
        color = ambient + Lo + texture(emissiveSampler, inUV0).rgb;
    } else {
        color = ambient + Lo;
    }

    // Gamma correct
    color = pow(color, vec3(0.4545));

    outColor = vec4(color, 1.0);
}
