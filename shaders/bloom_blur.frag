#version 410 core
// Phase 14 — Separable 9-tap Gaussian blur.
// uHorizontal = true  → blur along X  (first pass)
// uHorizontal = false → blur along Y  (second pass)
// Running N horizontal+vertical iterations (ping-pong FBOs) gives a wide blur.
in vec2 vUV;
out vec4 fragColor;

uniform sampler2D uImage;
uniform bool      uHorizontal;

// Gaussian weights for σ≈2.0: center + 4 offsets each side
const float weight[5] = float[](0.2270270270, 0.1945945946,
                                 0.1216216216, 0.0540540541,
                                 0.0162162162);

void main()
{
    vec2 texel = 1.0 / vec2(textureSize(uImage, 0));
    vec3 result = texture(uImage, vUV).rgb * weight[0];
    vec2 dir = uHorizontal ? vec2(texel.x, 0.0) : vec2(0.0, texel.y);
    for (int i = 1; i < 5; ++i) {
        result += texture(uImage, vUV + dir * float(i)).rgb * weight[i];
        result += texture(uImage, vUV - dir * float(i)).rgb * weight[i];
    }
    fragColor = vec4(result, 1.0);
}
