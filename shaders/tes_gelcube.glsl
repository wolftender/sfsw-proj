#version 410

layout (quads) in;

in TCS_OUT {
    vec2 uv;
} tes_in[];

out TES_OUT {
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

vec2 bernstein_uv (vec2 p0, vec2 p1, vec2 p2, vec2 p3, float t) {
    vec2 res;
    res.x = decasteljeu (p0.x, p1.x, p2.x, p3.x, t);
    res.y = decasteljeu (p0.y, p1.y, p2.y, p3.y, t);
    return res;
}

vec2 tx (int row, int col) {
    return tes_in[loc(row, col)].uv.xy;
}

vec2 bernstein_grid_uv (float u, float v) {
    vec2 p0, p1, p2, p3;

    p0 = bernstein_uv (tx(0,0),tx(0,1),tx(0,2),tx(0,3),u);
    p1 = bernstein_uv (tx(1,0),tx(1,1),tx(1,2),tx(1,3),u);
    p2 = bernstein_uv (tx(2,0),tx(2,1),tx(2,2),tx(2,3),u);
    p3 = bernstein_uv (tx(3,0),tx(3,1),tx(3,2),tx(3,3),u);

    return bernstein_uv (p0, p1, p2, p3, v);
}

void main () {
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;

    vec4 pos = vec4 (bernstein_grid (u, v), 1.0);

    gl_Position = u_projection * u_view * pos;
    tes_out.uv = bernstein_grid_uv(u,v);
}