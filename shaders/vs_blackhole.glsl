#version 330

layout (location = 0) in vec3 a_position;

uniform mat4 u_transform;

out VS_OUT {
    vec2 ndc_pos;
} vs_out;

void main() {
    gl_Position = vec4(a_position.xyz, 1.0);
}
