#version 410 core
// Phase 12 + 15 — Deferred lighting pass.
// Reads the 4 GBuffer textures and computes per-fragment lighting entirely on
// the screen quad — no scene geometry is drawn here.
//
// uUsePBR = true  → Cook-Torrance BRDF (GGX/Smith/Schlick) — Phase 15
// uUsePBR = false → Blinn-Phong (same model as lighting.frag)  — Phase 12
// Shadow mapping reconstructed from GBuffer world position    — Phase 10/12

in vec2 vUV;
out vec4 fragColor;

// GBuffer
uniform sampler2D gPosition;    // unit 0
uniform sampler2D gNormal;      // unit 1
uniform sampler2D gAlbedoSpec;  // unit 2
uniform sampler2D gMaterial;    // unit 3
// Shadow
uniform sampler2D uShadowMap;   // unit 4
uniform mat4      uLightSpaceMatrix;
// Camera
uniform vec3 uViewPos;
// Flags
uniform bool  uUsePBR = false;
uniform int   uDebugView = 0;   // 0=final,1=pos,2=normal,3=albedo,4=material,5=shadow

// ---------- Lights ----------
struct DirLight {
    vec3 direction;
    vec3 ambient, diffuse, specular;
};
struct PointLight {
    vec3 position;
    vec3 ambient, diffuse, specular;
    float constant, linear, quadratic;
};
struct SpotLight {
    vec3 position, direction;
    float cutOff, outerCutOff;
    vec3 ambient, diffuse, specular;
    float constant, linear, quadratic;
};

uniform DirLight    uDirLight;
uniform PointLight  uPointLights[4];
uniform int         uNumPointLights;
uniform bool        uUseSpotLight;
uniform SpotLight   uSpotLight;

// =========================================================================
// Shadow helpers  (identical to lighting.frag PCF)
// =========================================================================
float calcShadow(vec4 fragLS, vec3 normal, vec3 lightDir)
{
    vec3 proj = fragLS.xyz / fragLS.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 0.0;

    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y) {
            float depth = texture(uShadowMap,
                                  proj.xy + vec2(x,y)*texelSize).r;
            shadow += (proj.z - bias > depth) ? 1.0 : 0.0;
        }
    return shadow / 9.0;
}

// =========================================================================
// Blinn-Phong helpers
// =========================================================================
float attenuation(float dist, float c, float l, float q)
{
    return 1.0 / (c + l*dist + q*dist*dist);
}

vec3 bpDirLight(DirLight light, vec3 N, vec3 V, vec3 albedo,
                float specI, float shininess, float shadow)
{
    vec3 L    = normalize(-light.direction);
    vec3 H    = normalize(L + V);
    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), shininess);
    vec3 ambient  = light.ambient  * albedo;
    vec3 diffuse  = light.diffuse  * diff * albedo;
    vec3 specular = light.specular * spec * specI;
    return ambient + (1.0 - shadow) * (diffuse + specular);
}

vec3 bpPointLight(PointLight light, vec3 N, vec3 V, vec3 fragPos,
                  vec3 albedo, float specI, float shininess)
{
    vec3 L    = normalize(light.position - fragPos);
    vec3 H    = normalize(L + V);
    float dist = length(light.position - fragPos);
    float att  = attenuation(dist, light.constant, light.linear, light.quadratic);
    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), shininess);
    return att * (light.ambient  * albedo
                + light.diffuse  * diff * albedo
                + light.specular * spec * specI);
}

vec3 bpSpotLight(SpotLight light, vec3 N, vec3 V, vec3 fragPos,
                 vec3 albedo, float specI, float shininess)
{
    vec3 L    = normalize(light.position - fragPos);
    float theta   = dot(L, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intense = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    vec3 H    = normalize(L + V);
    float dist = length(light.position - fragPos);
    float att  = attenuation(dist, light.constant, light.linear, light.quadratic);
    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), shininess);
    return att * intense * (light.ambient  * albedo
                           + light.diffuse  * diff * albedo
                           + light.specular * spec * specI);
}

// =========================================================================
// PBR — Cook-Torrance BRDF (Phase 15)
// =========================================================================
const float PI = 3.14159265359;

float D_GGX(float NdotH, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 0.0001);
}

float G_SchlickGGX(float NdotX, float roughness)
{
    float r = roughness + 1.0;
    float k = (r*r) / 8.0;
    return NdotX / max(NdotX * (1.0 - k) + k, 0.0001);
}

float G_Smith(float NdotV, float NdotL, float roughness)
{
    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

vec3 F_Schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Returns radiance contribution for one light.
vec3 pbrLight(vec3 N, vec3 V, vec3 L, vec3 albedo,
              float metallic, float roughness, vec3 F0, vec3 radiance)
{
    vec3 H      = normalize(V + L);
    float NdotV = max(dot(N, V), 0.0001);
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    float D  = D_GGX(NdotH, roughness);
    float G  = G_Smith(NdotV, NdotL, roughness);
    vec3  F  = F_Schlick(HdotV, F0);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    vec3 specular = (D * G * F) / (4.0 * NdotV * NdotL + 0.0001);
    vec3 diffuse  = kD * albedo / PI;

    return (diffuse + specular) * radiance * NdotL;
}

// =========================================================================
// main
// =========================================================================
void main()
{
    vec3  fragPos  = texture(gPosition,   vUV).rgb;
    vec3  N        = normalize(texture(gNormal,    vUV).rgb);
    vec4  albSpec  = texture(gAlbedoSpec, vUV);
    vec4  matVals  = texture(gMaterial,   vUV);

    vec3  albedo    = albSpec.rgb;
    float specI     = albSpec.a;
    float metallic  = matVals.r;
    float roughness = matVals.g;
    float ao        = matVals.b;
    float shininess = matVals.a * 256.0 + 1.0;

    vec3 V = normalize(uViewPos - fragPos);

    // Shadow
    vec4  fragLS = uLightSpaceMatrix * vec4(fragPos, 1.0);
    vec3  Ldir   = normalize(-uDirLight.direction);
    float shadow = calcShadow(fragLS, N, Ldir);

    // Debug views
    if (uDebugView == 1) { fragColor = vec4(fragPos * 0.1 + 0.5, 1.0); return; }
    if (uDebugView == 2) { fragColor = vec4(N * 0.5 + 0.5, 1.0);       return; }
    if (uDebugView == 3) { fragColor = vec4(albedo, 1.0);               return; }
    if (uDebugView == 4) { fragColor = vec4(metallic, roughness, ao, 1.0); return; }
    if (uDebugView == 5) { fragColor = vec4(vec3(1.0 - shadow), 1.0);   return; }

    vec3 Lo = vec3(0.0);

    if (!uUsePBR) {
        // ---- Blinn-Phong ----
        Lo += bpDirLight(uDirLight, N, V, albedo, specI, shininess, shadow);
        for (int i = 0; i < uNumPointLights; ++i)
            Lo += bpPointLight(uPointLights[i], N, V, fragPos, albedo, specI, shininess);
        if (uUseSpotLight)
            Lo += bpSpotLight(uSpotLight, N, V, fragPos, albedo, specI, shininess);
    } else {
        // ---- Cook-Torrance PBR ----
        // F0: 0.04 for dielectrics, albedo-tinted for metals
        vec3 F0 = mix(vec3(0.04), albedo, metallic);

        // Directional light
        vec3 radiance = uDirLight.diffuse;
        Lo += pbrLight(N, V, Ldir, albedo, metallic, roughness, F0, radiance)
              * (1.0 - shadow);
        // Ambient from dir light direction (no shadow)
        Lo += uDirLight.ambient * albedo * ao;

        // Point lights
        for (int i = 0; i < uNumPointLights; ++i) {
            vec3  Lp  = normalize(uPointLights[i].position - fragPos);
            float dist = length(uPointLights[i].position - fragPos);
            float att  = 1.0 / (uPointLights[i].constant
                               + uPointLights[i].linear    * dist
                               + uPointLights[i].quadratic * dist * dist);
            Lo += pbrLight(N, V, Lp, albedo, metallic, roughness, F0,
                           uPointLights[i].diffuse * att);
        }

        // Spot light
        if (uUseSpotLight) {
            vec3  Ls    = normalize(uSpotLight.position - fragPos);
            float theta   = dot(Ls, normalize(-uSpotLight.direction));
            float epsilon = uSpotLight.cutOff - uSpotLight.outerCutOff;
            float intens  = clamp((theta - uSpotLight.outerCutOff)/epsilon, 0.0, 1.0);
            float dist  = length(uSpotLight.position - fragPos);
            float att   = 1.0 / (uSpotLight.constant
                                + uSpotLight.linear    * dist
                                + uSpotLight.quadratic * dist * dist);
            Lo += pbrLight(N, V, Ls, albedo, metallic, roughness, F0,
                           uSpotLight.diffuse * att * intens);
        }
    }

    fragColor = vec4(Lo, 1.0);
}
