#version 120

uniform sampler2D sampler;
uniform int mode;

varying vec2 fragment_uv;

float near = 0.125;
float far  = 64.0;

float nominal_a = -1.966041;
float fX = -0.0126;
float fY = 0.0001;
float C0 = 0.0193;
float lbda = 530e-09;
float f0 = 100e-03;
float fe = 40e-03;
float SLMpitch = 3.74e-06;
float W = 4.0;

float slmWidth = 4000;
float slmHeight = 2464;

void main() {
    vec4 depth = texture2D(sampler, fragment_uv);

    if (mode == 0) { // Minecraft mode
        vec4 ndc = depth * 2.0 - 1.0;
        depth = (2.0 * near * far) / (far + near - ndc * (far - near));

        near = 4;
        far = 64;

        // Clip depths
        depth = max(depth,near);
        depth = min(depth,far);

        depth = 1 / depth;

        // Map to [0, 1]
        depth = depth - 1.0 / far;
        depth = depth / (1.0 / near - 1.0 / far);
    }

    vec4 diopterMap = floor(depth * 50) / 50.0;

    diopterMap = diopterMap * W;

    float u = slmWidth * (fragment_uv.x - 0.5);
    float v = slmHeight * (fragment_uv.y - 0.5);

    float factorY = nominal_a / sqrt(1 + nominal_a * nominal_a);
    vec4 scaleY = ((C0*SLMpitch*fe*fe) / (3*lbda*f0*f0*f0)) * (W / 2 - diopterMap);
    vec4 scaleX = scaleY / nominal_a;
    vec4 DeltaX = -scaleX * ((lbda * f0) / (2 * SLMpitch));
    vec4 DeltaY = -scaleY * ((lbda * f0) / (2 * SLMpitch));
    float N = (lbda * f0) / SLMpitch;

    vec4 thetaX = DeltaX / N;
    vec4 thetaY = DeltaY / N;
    vec4 phaseData = mod((thetaX * u + thetaY * v)+((fX * u + fY * v)), 1);

    gl_FragColor = vec4(phaseData);
    gl_FragColor.a = 1;

    //gl_FragColor = depth;
    //gl_FragColor.a = 1;
}
