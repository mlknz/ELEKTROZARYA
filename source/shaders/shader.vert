#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUv0;
layout(location = 3) in vec2 inUv1;

layout(location = 0) out vec2 uv;

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 viewProjectionMatrix;
} globalUniforms;

layout(push_constant) uniform PushConstantsObject {
  mat4 modelMatrix;
} pushConstants;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = globalUniforms.viewProjectionMatrix * pushConstants.modelMatrix * vec4(inPosition, 1.0);
    uv = inUv0;
}
