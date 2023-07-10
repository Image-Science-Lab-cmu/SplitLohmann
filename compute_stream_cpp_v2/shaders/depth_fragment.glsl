#version 120

uniform sampler2D sampler;

float near = 0.125, far = 384;


void main() {
    float depth = texture2D(sampler, gl_TexCoord[0].st).x;

    //gl_FragColor = vec4(depth);

    float ndc = depth * 2.0 - 1.0;
    depth = (2.0 * near * far) / (far + near - ndc * (far - near));

    //gl_FragColor = vec4(sin(1*depth)*0.5+0.5);
    gl_FragColor = vec4(depth / far);
}
