#version 450

const vec3 cube[4 * 6] = {
    // NORTH FACE:
    vec3(0, 1, 1),
    vec3(0, 0, 1),
    vec3(1, 1, 1),
    vec3(1, 0, 1),
    // SOUTH FACE:
    vec3(1, 1, 0),
    vec3(1, 0, 0),
    vec3(0, 1, 0),
    vec3(0, 0, 0),
    // WEST FACE:
    vec3(1, 1, 1),
    vec3(1, 0, 1),
    vec3(1, 1, 0),
    vec3(1, 0, 0),
    // EAST FACE:
    vec3(0, 1, 0),
    vec3(0, 0, 0),
    vec3(0, 1, 1),
    vec3(0, 0, 1),
    // TOP FACE:
    vec3(1, 1, 1),
    vec3(1, 1, 0),
    vec3(0, 1, 1),
    vec3(0, 1, 0),
    // BOTTOM FACE:
    vec3(0, 0, 1),
    vec3(0, 0, 0),
    vec3(1, 0, 1),
    vec3(1, 0, 0),
};

const vec2 texcoords[4] = {
    vec2(0, 0),
    vec2(0, 1),
    vec2(1, 0),
    vec2(1, 1),
};

layout(binding = 0) uniform UBO {
    mat4 mvp;
} ubo;

layout(push_constant) uniform PCONST {
    uint texture_types;
} pc;

layout(location = 0) in uint direction;
layout(location = 1) in uint texture_id;
layout(location = 2) in ivec3 position;

layout(location = 0) out vec2 texcoord;
layout(location = 1) out uint out_direction;

void main() {
    gl_Position = ubo.mvp * vec4(cube[direction * 4 + gl_VertexIndex] + position, 1);
    texcoord = texcoords[gl_VertexIndex];
    texcoord.y = (texcoord.y + texture_id) / pc.texture_types;
    out_direction = direction;
}
