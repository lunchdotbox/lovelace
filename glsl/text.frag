#version 450
#include "bezier.glsl"

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 outColor;

void main() {
    float sdf = sdBezier(in_uv, vec2(0.25, 0.75), vec2(0.5, 0.5), vec2(0.75, 0.75));

    outColor = vec4(0.0);
    if (sdf < 0.01) outColor = vec4(1.0);
}
