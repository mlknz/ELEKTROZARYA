#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 direction;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform samplerCube envCubemap;

void main()
{
    const vec3 cubemapSample = texture(envCubemap, normalize(direction)).rgb;

    outColor = vec4(cubemapSample, 1.0);
}
