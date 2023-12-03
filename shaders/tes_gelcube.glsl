#version 410

layout (quads) in;

out TES_OUT {
    vec3 world_pos;
    vec3 normal;
    vec3 tangent;
    vec2 uv;
} tes_out;

uniform mat4 u_world;
uniform mat4 u_view;
uniform mat4 u_projection;

float decasteljeu (float b00, float b01, float b02, float b03, float t) {
    float t1 = t;
    float t0 = 1.0 - t;

    float b10, b11, b12;
    float b20, b21;
    float b30;

    b10 = t0 * b00 + t1 * b01;
    b11 = t0 * b01 + t1 * b02;
    b12 = t0 * b02 + t1 * b03;

    b20 = t0 * b10 + t1 * b11;
    b21 = t0 * b11 + t1 * b12;

    b30 = t0 * b20 + t1 * b21;

    return b30;
}

vec3 der_decasteljeu(vec3 b00, vec3 b01, vec3 b02, vec3 b03, float t) {
        float t1 = t;
		float t0 = 1.0 - t;

		vec3 d10 = -3.0f * b00 + 3.0f * b01;
		vec3 d11 = -3.0f * b01 + 3.0f * b02;
		vec3 d12 = -3.0f * b02 + 3.0f * b03;

		vec3 d20 = t0 * d10 + t1 * d11;
		vec3 d21 = t0 * d11 + t1 * d12;

		vec3 d30 = t0 * d20 + t1 * d21;
		return d30;
}

vec3 bernstein (vec3 p0, vec3 p1, vec3 p2, vec3 p3, float t) {
    vec3 res;
    res.x = decasteljeu (p0.x, p1.x, p2.x, p3.x, t);
    res.y = decasteljeu (p0.y, p1.y, p2.y, p3.y, t);
    res.z = decasteljeu (p0.z, p1.z, p2.z, p3.z, t);
    return res;
}

int loc (int row, int col) {
    return (row * 4) + col;
}

vec3 p (int row, int col) {
    return gl_in[loc(row, col)].gl_Position.xyz;
}

vec3 bernstein_grid (float u, float v) {
    vec3 p0, p1, p2, p3;

    p0 = bernstein (p(0,0),p(0,1),p(0,2),p(0,3),u);
    p1 = bernstein (p(1,0),p(1,1),p(1,2),p(1,3),u);
    p2 = bernstein (p(2,0),p(2,1),p(2,2),p(2,3),u);
    p3 = bernstein (p(3,0),p(3,1),p(3,2),p(3,3),u);

    return bernstein (p0, p1, p2, p3, v);
}

vec3 surface_ddu(float u, float v) {
    vec3 p00 = bernstein(p(0,0),p(0,1),p(0,2),p(0,3),u);
    vec3 p01 = bernstein(p(1,0),p(1,1),p(1,2),p(1,3),u);
    vec3 p02 = bernstein(p(2,0),p(2,1),p(2,2),p(2,3),u);
    vec3 p03 = bernstein(p(3,0),p(3,1),p(3,2),p(3,3),u);

    return der_decasteljeu(p00, p01, p02, p03, v);
}

vec3 surface_ddv(float u, float v) {
    vec3 p10 = bernstein(p(0,0),p(0,0),p(2,0),p(3,0),v);
    vec3 p11 = bernstein(p(0,1),p(0,1),p(2,1),p(3,1),v);
    vec3 p12 = bernstein(p(0,2),p(0,2),p(2,2),p(3,2),v);
    vec3 p13 = bernstein(p(0,3),p(0,3),p(2,3),p(3,3),v);

    return der_decasteljeu(p10, p11, p12, p13, u);
}

void main () {
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;

    vec4 pos = vec4 (bernstein_grid (u, v), 1.0);

    gl_Position = u_projection * u_view * pos;

    vec3 ddu = surface_ddu(u,v);
    vec3 ddv = surface_ddv(u,v);

    tes_out.world_pos = pos.xyz;
    tes_out.uv = vec2(u,v);
    tes_out.normal = normalize(cross(ddu,ddv));
    tes_out.tangent = normalize(ddu);
}