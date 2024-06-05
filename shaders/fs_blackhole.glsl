#version 330

in VS_OUT {
    vec2 ndc_pos;
} fs_in;

void main() {
    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
