#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D baseColorSampler;
layout(set = 1, binding = 1) uniform sampler2D metallicRoughnessSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampler;
layout(set = 1, binding = 3) uniform sampler2D occlusionSampler;
layout(set = 1, binding = 4) uniform sampler2D emissionSampler;

void main() {
    outColor = texture(baseColorSampler, uv);
}
