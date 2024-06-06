#version 330

in VS_OUT {
    vec3 ndc_pos;
} fs_in;

uniform mat4 u_world;
uniform mat4 u_view;
uniform mat4 u_projection;

uniform vec2 u_resolution;
uniform samplerCube u_sampler;

uniform float u_star_mass;
uniform float u_distance;

out vec4 frag_color;

// rotate vector v around axis k by theta degrees
vec3 rodrigues(vec3 v, vec3 k, float theta) {
    float c = cos(theta);
    float s = sin(theta);
    
    return v*c + cross(k,v)*s + k*dot(k,v)*(1.0 - c);
}

// assume that the black hole is in (0,0,0)
// assume that the camera distance is u_distance
// assume that the black hole M = u_star_mass

void main() {
    // we assume that world is a rotation
    // so the inverse is simply a transposition
    mat4 world_inv = transpose(u_world);
    float aspect = u_resolution.x / u_resolution.y;

    vec3 cam_position = vec3(0.0, 0.0, -1.8);
    vec3 view_pos = vec3(fs_in.ndc_pos.x * aspect, -fs_in.ndc_pos.y, 0.0);
    vec3 ray_dir = normalize(view_pos - cam_position);
    //vec3 world_dir = (u_world * vec4(ray_dir, 0.0)).xyz;
    
    //vec3 sky = texture(u_sampler, world_dir).xyz;
    vec3 star_worldpos = vec3(0.0, 0.0, 0.0);
    vec3 cam_worldpos = vec3(0.0, 0.0, -u_distance);

    // equation of a line in here is x = a + tn
    // where a is camera worldpos, n is ray direction
    float t0 = dot(star_worldpos - cam_worldpos, ray_dir);
    vec3 p0 = cam_worldpos + t0 * ray_dir;

    // impact parameter
    float b = length(p0);
    float bsq = b * b;
    float M = u_star_mass;
    float Msq = M * M;

    if (1.0/bsq < 1.0/(Msq * 27.0)) {
        // TODO: replace with real integral
        float dtheta = (1.0 / (1.0 / sqrt(27.0) - M / b) - sqrt(27.0)) / 12.0;

        vec3 left = normalize(p0 - star_worldpos);
        vec3 forward = normalize(star_worldpos - cam_worldpos);
        vec3 up = cross(left, forward);
        vec3 final_dir = rodrigues(ray_dir, up, dtheta);
        vec3 world_dir = (u_world * vec4(final_dir, 0.0)).xyz;
        vec3 sky = texture(u_sampler, world_dir).xyz;

        frag_color = vec4(sky, 1.0);
    } else {
        frag_color = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
