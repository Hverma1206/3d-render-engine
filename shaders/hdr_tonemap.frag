#version 410 core
// Phase 13 + 14 — HDR tone mapping + bloom composite + gamma correction.
// uTonemapMode: 0 = Reinhard, 1 = ACES Filmic
in vec2 vUV;
out vec4 fragColor;

uniform sampler2D uHDR;        // HDR colour buffer
uniform sampler2D uBloom;      // blurred bloom buffer
uniform float     uExposure    = 1.0;
uniform bool      uBloomOn     = true;
uniform int       uTonemapMode = 1;      // 0=Reinhard, 1=ACES

// ACES Filmic approximation (Krzysztof Narkowicz)
vec3 ACES(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main()
{
    vec3 hdr   = texture(uHDR,   vUV).rgb;
    vec3 bloom = texture(uBloom, vUV).rgb;

    if (uBloomOn) hdr += bloom;

    // Exposure
    hdr *= uExposure;

    // Tone mapping
    vec3 ldr;
    if (uTonemapMode == 0)
        ldr = hdr / (hdr + 1.0);           // Reinhard
    else
        ldr = ACES(hdr);                    // ACES Filmic

    // Gamma correction (linear → sRGB)
    ldr = pow(ldr, vec3(1.0 / 2.2));

    fragColor = vec4(ldr, 1.0);
}
