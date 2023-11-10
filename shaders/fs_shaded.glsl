#version 330

in VS_OUT {
    vec3 local_pos;
    vec3 world_pos;
    vec3 view_pos;
    vec2 uv;
    vec4 color;
    vec3 normal;
    vec3 world_normal;
} fs_in;

uniform sampler2D u_texture;

out vec4 frag_color;

void main() {
    frag_color = vec4(fs_in.color.xyz, 1.0);
}