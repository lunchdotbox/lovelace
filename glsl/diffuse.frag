#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#include "push.glsl"

layout(binding = 0) uniform sampler2D textures[];

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(textures[push.texture_id], in_uv);
    // outColor = vec4(in_uv, 0.0, 1.0);
}
