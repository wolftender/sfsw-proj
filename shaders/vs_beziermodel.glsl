#version 330

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;

uniform mat4 u_world;
uniform mat4 u_view;
uniform mat4 u_projection;

out VS_OUT {
    vec3 local_pos;
    vec3 world_pos;
    vec3 view_pos;
    vec2 uv;
    vec3 normal;
    vec3 world_normal;
} vs_out;

void main () {
    vec4 world_pos = u_world * vec4 (a_position, 1.0);
    vec4 view_pos = u_view * world_pos;

    vs_out.uv = a_uv;
    vs_out.normal = a_normal;
    vs_out.world_normal = normalize((u_world * vec4(a_normal, 0.0)).xyz);

    vs_out.world_pos = world_pos.xyz;
    vs_out.view_pos = view_pos.xyz; 

    gl_Position = u_projection * view_pos;
}