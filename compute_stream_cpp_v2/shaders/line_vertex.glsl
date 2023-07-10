#version 120

uniform mat4 matrix;
uniform float baseline;

attribute vec4 position;

void main() {
    gl_Position = matrix * position;
    gl_Position[0] += baseline;
}
