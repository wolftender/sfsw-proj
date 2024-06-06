#version 330

in VS_OUT {
    vec3 ndc_pos;
} fs_in;

uniform mat4 u_world;
uniform mat4 u_view;
uniform mat4 u_projection;

uniform samplerCube u_sampler;

out vec4 frag_color;

void main() {
    vec3 cam_position = vec3(0.0, 0.0, -1.8);
    vec3 view_pos = vec3(fs_in.ndc_pos.xy, 0.0);
    vec3 ray_dir = normalize(view_pos - cam_position);
    
    ray_dir = (u_world * vec4(ray_dir, 0.0)).xyz;
    vec3 sky = texture(u_sampler, ray_dir).xyz;

    frag_color = vec4(sky, 1.0);
}
