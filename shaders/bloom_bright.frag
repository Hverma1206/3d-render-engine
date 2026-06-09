#version 410 core
// Phase 14 — Bright-pass filter.
// Extracts fragments brighter than uThreshold (by luminance) for bloom.
in vec2 vUV;
out vec4 fragColor;

uniform sampler2D uHDR;
uniform float uThreshold = 1.0;

void main()
{
    vec3 color = texture(uHDR, vUV).rgb;
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    // Soft knee: smooth extraction avoids hard edge on bright objects
    float weight = max(luminance - uThreshold, 0.0) / max(luminance, 0.0001);
    fragColor = vec4(color * weight, 1.0);
}
