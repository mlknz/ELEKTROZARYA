#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 direction;

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 viewProjectionMatrix;
} globalUniforms;

void main()
{
    const vec3 position = inPosition;
    mat3 viewRotScale = mat3(globalUniforms.viewMatrix);
    const vec4 projectedPosition = globalUniforms.projectionMatrix * mat4(viewRotScale) * vec4(inPosition, 1.0);

    direction = position;
    gl_Position = projectedPosition.xyww;
}
