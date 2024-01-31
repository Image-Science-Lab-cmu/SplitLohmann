#version 120

uniform mat4 matrix;

attribute vec4 position;
attribute vec2 uv;

varying vec2 fragment_uv;

void main() {
    mat3 homo = mat3(
        7.21986816e-03, -1.93139571e+00,  4.48421146e+03,
        1.92107275e+00,  8.71735526e-03, -1.54284033e+03,
       -2.57746592e-06, -2.31344956e-06,  1.00557923e+00);

    mat3 homo_inv = mat3(
        1.40315760e-03,  5.21602553e-01,  7.94027338e+02,
       -5.20527059e-01,  5.08104200e-03,  2.32899862e+03,
       -1.19393526e-06,  1.34864315e-06,  1.00184507e+00);

    mat3 oled = mat3(0.0);
    oled[0][0] = 1/2560.0;
    oled[1][1] = 1/2560.0;
    oled[2][2] = 1.0;

    mat3 slm = mat3(0.0);
    slm[0][0] = 4000.0;
    slm[1][1] = 2464.0;
    slm[2][2] = 1.0;

    gl_Position.xyw = oled * transpose(homo_inv) * slm * position.xyw;
    gl_Position.z = 0;
    gl_Position = matrix * gl_Position;

    fragment_uv = uv;
}
