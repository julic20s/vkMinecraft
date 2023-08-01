#version 450

layout(binding = 1) uniform sampler2D tex_sampler;

layout(location = 0) in vec2 texcoord;
layout(location = 1) flat in uint direction;
layout(location = 0) out vec4 color;

const float face_color[6] = {
    0.7, 0.85, 0.7, 0.85, 1, 0.5
};

void main() {
    color = vec4(texture(tex_sampler, texcoord).rgb * face_color[direction], 1);
}
