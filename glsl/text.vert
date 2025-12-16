#version 450

layout(location = 0) out vec2 out_uv;

const vec2 quad_vertices[] = {
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0),
        vec2(1.0, 1.0),
        vec2(0.0, 1.0),
        vec2(0.0, 0.0),
    };

void main() {
    vec2 vertex = quad_vertices[gl_VertexIndex];

    gl_Position = vec4(vertex, 0.0, 1.0);
    out_uv = vertex;
}
