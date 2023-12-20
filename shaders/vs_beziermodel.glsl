#version 330

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;

uniform mat4 u_world;
uniform mat4 u_view;
uniform mat4 u_projection;

uniform vec3 u_control_points[64];

out VS_OUT {
    vec3 local_pos;
    vec3 world_pos;
    vec3 view_pos;
    vec2 uv;
    vec3 normal;
    vec3 world_normal;
} vs_out;

vec3 decasteljeu(vec3 b00, vec3 b01, vec3 b02, vec3 b03, float t) {
    float t1 = t;
    float t0 = 1.0 - t;

    vec3 b10, b11, b12;
    vec3 b20, b21;
    vec3 b30;

    b10 = t0 * b00 + t1 * b01;
    b11 = t0 * b01 + t1 * b02;
    b12 = t0 * b02 + t1 * b03;

    b20 = t0 * b10 + t1 * b11;
    b21 = t0 * b11 + t1 * b12;

    b30 = t0 * b20 + t1 * b21;

    return b30;
}

int loc(int mx, int my, int mz) {
    return (mx * 16) + (my * 4) + mz;
}

vec3 p(int mx, int my, int mz) {
    return u_control_points[loc(mx, my, mz)];
}

vec3 bezier_cube(vec3 coords) {
    vec3 p1[4];
    vec3 p2[4];
    vec3 p3[4];

    for (int mz = 0; mz < 4; ++mz) {
        for (int my = 0; my < 4; ++my) {
            for (int mx = 0; mx < 4; ++mx) {
                p1[mx] = p(mx,my,mz);
            }
            p2[my] = decasteljeu(p1[0],p1[1],p1[2],p1[3], coords.x);
        }
        p3[mz] = decasteljeu(p2[0],p2[1],p2[2],p2[3], coords.y);
    }

    return decasteljeu(p3[0],p3[1],p3[2],p3[3], coords.z);
}

void main () {
    vs_out.local_pos = a_position.xyz;
    vec3 coords = (a_position.xyz + 1.0) * 0.5;

    vec4 world_pos = vec4(bezier_cube(coords), 1.0f);
    vec4 view_pos = u_view * world_pos;

    vs_out.uv = a_uv;
    vs_out.normal = a_normal;

    // mapping normals to curvilinear space
    //  1. take a POINT that is current surface point plus very small offset in normal direction
    //  2. map this point using bezier cube mapping
    //  3. read the normal back
    vec3 offset_pos = a_position.xyz + 0.05 * a_normal.xyz;
    vec3 coords_norm = (offset_pos.xyz + 1.0) * 0.5;
    vec3 offset_mapped = bezier_cube(coords_norm);
    vec3 world_normal = normalize(offset_mapped - world_pos.xyz);
    vs_out.world_normal = world_normal;

    vs_out.world_pos = world_pos.xyz;
    vs_out.view_pos = view_pos.xyz; 

    gl_Position = u_projection * view_pos;
}