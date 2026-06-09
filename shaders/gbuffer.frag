#version 410 core
// Phase 12 — GBuffer geometry pass fragment shader.
// Writes position, normal, albedo+specular, and PBR material to 4 render targets.
// Phase 15 adds metallic/roughness/ao written to RT3.

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vUV;

layout(location = 0) out vec4 gPosition;     // world position (xyz)
layout(location = 1) out vec4 gNormal;       // world normal (xyz, normalised)
layout(location = 2) out vec4 gAlbedoSpec;   // albedo (rgb) + specular intensity (a)
layout(location = 3) out vec4 gMaterial;     // metallic (r), roughness (g), ao (b)

uniform sampler2D uDiffuseTex;
uniform sampler2D uSpecularTex;
uniform float     uMetallic   = 0.0;
uniform float     uRoughness  = 0.5;
uniform float     uAO         = 1.0;
uniform float     uShininess  = 32.0;

void main()
{
    gPosition   = vec4(vFragPos, 1.0);
    gNormal     = vec4(normalize(vNormal), 0.0);

    vec3  albedo  = texture(uDiffuseTex,  vUV).rgb;
    float spec    = texture(uSpecularTex, vUV).r;
    gAlbedoSpec   = vec4(albedo, spec);

    gMaterial     = vec4(uMetallic, uRoughness, uAO, uShininess / 256.0);
}
