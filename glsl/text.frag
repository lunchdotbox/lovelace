#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(binding = 0) uniform sampler2D textures[];

layout(push_constant) uniform Push {
    uint texture_id;
    uint transform_buffer_id;
    float aspect;
} push;

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_tint;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(textures[push.texture_id], in_uv) * in_tint;
}
