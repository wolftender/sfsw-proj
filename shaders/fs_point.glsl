#version 330

in vec4 vertex_color;
in vec2 uv;

out vec4 output_color;

uniform vec4 u_color;

void main () {
    vec2 p = uv - 0.5;
    float l = p.x*p.x + p.y*p.y;

    output_color = u_color * smoothstep(1.0, 0.0, 5.0 * l);
}