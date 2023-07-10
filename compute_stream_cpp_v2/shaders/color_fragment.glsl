#version 120

uniform sampler2D sampler;

void main() {
    vec4 color = texture2D(sampler, gl_TexCoord[0].st);
    gl_FragColor = color;
}
