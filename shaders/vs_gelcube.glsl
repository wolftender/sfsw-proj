#version 410

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;

out VS_OUT {
    vec2 uv;
} vs_out;

void main () {
    gl_Position = vec4 (a_position, 1.0);
    vs_out.uv = a_uv;
}