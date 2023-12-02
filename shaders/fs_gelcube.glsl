#version 410

in TES_OUT {
    vec2 uv;
} fs_in;

out vec4 output_color;

uniform sampler2D u_texture;
uniform vec4 u_color;

void main () {
    output_color = u_color;
}