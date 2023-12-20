#version 330

struct point_light_t {
    vec3 color;
    vec3 position;

    float intensity;
    float att_const;
    float att_lin;
    float att_sq;
};

in VS_OUT {
    vec3 local_pos;
    vec3 world_pos;
    vec3 view_pos;
    vec2 uv;
    vec3 normal;
    vec3 world_normal;
} fs_in;

uniform point_light_t u_point_lights[10];
uniform vec3 u_ambient;
uniform vec3 u_camera_position;
uniform vec4 u_surface_color;
uniform float u_shininess;
uniform float u_gamma;
uniform bool u_wireframe;

uniform bool u_enable_albedo;
uniform sampler2D u_albedo_map;

out vec4 output_color;

vec4 phong (point_light_t light, vec3 viewer_direction, vec3 light_direction, vec3 normal) {
    vec4 diffuse = vec4 (0.0f);
    vec4 specular = vec4 (0.0f);

    // diffuse color
    float df = max (dot (normal, light_direction), 0.0);
    diffuse = light.intensity * df * vec4 (light.color, 1.0);

    vec3 reflected = normalize (reflect (-light_direction, normal));
    float sf = max (dot (viewer_direction, reflected), 0.0);
    
    if (u_shininess == 0.0) {
        sf = 0.0;
    } else {
        sf = pow (sf, u_shininess);
    }

    specular = light.intensity * sf * vec4 (light.color, 1.0);

    return (diffuse + specular);
}

vec4 gamma_correct (vec4 color, float gamma) {
    vec4 out_color = color;
    out_color.xyz = pow (out_color.xyz, vec3 (1.0 / gamma));

    return out_color;
}

vec3 get_tex_color() {
    if (u_enable_albedo) {
        return texture(u_albedo_map, fs_in.uv).rgb;
    } else {
        return u_surface_color.xyz;
    }
}

vec4 phong_shader() {
    vec4 phong_component = vec4 (0.0f);
    vec3 world_normal = fs_in.world_normal;

    for (int i = 0; i < 10; ++i) {
        if (u_point_lights[i].intensity > 0.0) {
            vec3 light_direction = u_point_lights[i].position - fs_in.world_pos.xyz;
            vec3 viewer_direction = u_camera_position - u_point_lights[i].position;

            float dist = length (light_direction);
            float att_inv = u_point_lights[i].att_const + u_point_lights[i].att_lin * dist + u_point_lights[i].att_sq * dist * dist;

            phong_component += phong (u_point_lights[i], normalize (viewer_direction), normalize (light_direction), world_normal) / att_inv;
        }
    }

    vec4 frag_color = vec4(u_ambient * get_tex_color(), 1.0);

    frag_color = gamma_correct (frag_color + phong_component, u_gamma);
    frag_color.w = u_surface_color.w;

    return frag_color;
}

void main () {
    if (u_wireframe) {
        output_color = vec4(u_surface_color.xyz, 1.0);
    } else {
        output_color = phong_shader();
    }
}