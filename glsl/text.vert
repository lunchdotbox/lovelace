#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(push_constant) uniform Push {
    uint texture_id;
    uint transform_buffer_id;
} push;

struct Character {
    vec4 transform1_and_character;
    vec3 transform2;
    vec3 transform3;
};

layout(binding = 1) readonly buffer Text {
    Character characters[];
} text[];

layout(location = 0) out vec2 out_uv;

const float CHAR_WIDTH = 1.0 / 13.0;
const float CHAR_HEIGHT = 1.0f / 7.0;

const vec2 quad_uvs[] = {
        vec2(0.0, 0.0),
        vec2(CHAR_WIDTH, 0.0),
        vec2(CHAR_WIDTH, CHAR_HEIGHT),
        vec2(CHAR_WIDTH, CHAR_HEIGHT),
        vec2(0.0, CHAR_HEIGHT),
        vec2(0.0, 0.0),
    };

const vec2 quad_vertices[] = {
        vec2(-1.0, -1.0),
        vec2(1.0, -1.0),
        vec2(1.0, 1.0),
        vec2(1.0, 1.0),
        vec2(-1.0, 1.0),
        vec2(-1.0, -1.0),
    };

void main() {
    Character character = text[push.transform_buffer_id].characters[gl_InstanceIndex];

    mat3 transform = mat3(
            character.transform1_and_character.xyz,
            character.transform2,
            character.transform3
        );

    uint letter = floatBitsToUint(character.transform1_and_character.w);

    vec2 uv_offset = vec2((letter % 13) * CHAR_WIDTH, trunc(letter / 13) * CHAR_HEIGHT);

    vec2 vertex = (transform * vec3(quad_vertices[gl_VertexIndex], 1.0f)).xy;
    vec2 uv = quad_uvs[gl_VertexIndex] + uv_offset;

    gl_Position = vec4(vertex, 0.0, 1.0);
    out_uv = uv;
}
