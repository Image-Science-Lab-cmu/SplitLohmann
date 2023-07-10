#version 120

uniform mat3 matrix;

void main() {
    vec4 position = ftransform();

    gl_Position.xyw = matrix * position.xyw;
    gl_Position.z = 0;

    gl_TexCoord[0] = gl_MultiTexCoord0;
}
