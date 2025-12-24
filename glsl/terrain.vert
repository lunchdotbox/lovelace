#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#include "push.glsl"

layout(binding = 2) uniform Ubo {
    mat4 view_matrix;
} ubos[];

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec2 out_uv;

void main() {
    gl_Position = ubos[push.uniform_id].view_matrix * push.model_matrix * vec4(in_position, 1.0);
    out_uv = in_uv;
}
