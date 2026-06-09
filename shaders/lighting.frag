#version 410 core

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vUV;
in vec4 vFragPosLightSpace;

out vec4 FragColor;

// ---------------------------------------------------------------------------
// Material
// ---------------------------------------------------------------------------
struct Material {
    sampler2D diffuse;    // unit 0
    sampler2D specular;   // unit 1
    float     shininess;
};
uniform Material uMaterial;

// ---------------------------------------------------------------------------
// Light types
// ---------------------------------------------------------------------------
struct DirLight {
    vec3 direction; // world-space direction the light travels (not the source pos)
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3  position;
    float constant;
    float linear;
    float quadratic;
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
};

struct SpotLight {
    vec3  position;
    vec3  direction;
    float cutOff;       // cos(inner cone half-angle)
    float outerCutOff;  // cos(outer cone half-angle)
    float constant;
    float linear;
    float quadratic;
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
};

#define MAX_POINT_LIGHTS 4
uniform DirLight   uDirLight;
uniform PointLight uPointLights[MAX_POINT_LIGHTS];
uniform int        uNumPointLights;
uniform SpotLight  uSpotLight;
uniform bool       uUseSpotLight;

uniform sampler2D  uShadowMap; // unit 2
uniform vec3       uViewPos;

// ---------------------------------------------------------------------------
// Shadow: PCF 3×3 kernel over the directional light's shadow map.
//
// HOW IT WORKS:
//   1. Transform the fragment to the light's clip space (done in vert shader).
//   2. Perspective divide → NDC, then remap [-1,1] → [0,1] for texture lookup.
//   3. Compare fragment depth (minus bias) against the stored shadow-map depth.
//   4. Average 9 samples (PCF) to produce a soft shadow edge.
//
// BIAS: without it, floating-point precision causes a fragment to shadow itself
// ("shadow acne").  Adaptive bias = larger on grazing surfaces, smaller on
// surfaces facing the light.  Too large = "peter-panning" (object floats).
// ---------------------------------------------------------------------------
float calcShadow(vec4 fragPosLS, vec3 norm, vec3 lightDir)
{
    // Perspective divide; for an orthographic light this is a no-op (w == 1).
    vec3 proj = fragPosLS.xyz / fragPosLS.w;
    proj = proj * 0.5 + 0.5; // [-1,1] -> [0,1] (shadow-map UV space)

    // Fragment beyond the light frustum far plane is never in shadow.
    if (proj.z > 1.0) return 0.0;

    float currentDepth = proj.z;
    float bias = max(0.05 * (1.0 - dot(norm, lightDir)), 0.005);

    // PCF: sample the 3×3 neighbourhood, average the binary shadow results.
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(uShadowMap, proj.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
        }
    return shadow / 9.0;
}

// ---------------------------------------------------------------------------
// Blinn-Phong lighting functions
// Uses the halfway vector H = normalize(L + V) instead of reflect(−L, N) for
// the specular term. Produces a slightly wider, softer highlight and avoids the
// cutoff artefact of the Phong model at large angles.
// ---------------------------------------------------------------------------
vec3 blinnPhong(vec3 lightDir, vec3 norm, vec3 viewDir,
                vec3 ambient, vec3 diffuse, vec3 specular,
                vec3 diffTex, vec3 specTex)
{
    float diff = max(dot(norm, lightDir), 0.0);
    vec3  half_ = normalize(lightDir + viewDir);
    float spec  = pow(max(dot(norm, half_), 0.0), uMaterial.shininess);
    return ambient * diffTex
         + diffuse  * diff * diffTex
         + specular * spec * specTex;
}

vec3 calcDirLight(DirLight l, vec3 norm, vec3 viewDir, vec3 diffTex, vec3 specTex)
{
    return blinnPhong(normalize(-l.direction), norm, viewDir,
                      l.ambient, l.diffuse, l.specular, diffTex, specTex);
}

vec3 calcPointLight(PointLight l, vec3 norm, vec3 fragPos, vec3 viewDir,
                    vec3 diffTex, vec3 specTex)
{
    vec3  lightDir = normalize(l.position - fragPos);
    float d        = length(l.position - fragPos);
    float atten    = 1.0 / (l.constant + l.linear * d + l.quadratic * d * d);
    return atten * blinnPhong(lightDir, norm, viewDir,
                              l.ambient, l.diffuse, l.specular, diffTex, specTex);
}

vec3 calcSpotLight(SpotLight l, vec3 norm, vec3 fragPos, vec3 viewDir,
                   vec3 diffTex, vec3 specTex)
{
    vec3  lightDir = normalize(l.position - fragPos);
    float theta    = dot(lightDir, normalize(-l.direction));
    float eps      = l.cutOff - l.outerCutOff;
    float intensity = clamp((theta - l.outerCutOff) / eps, 0.0, 1.0);
    float d        = length(l.position - fragPos);
    float atten    = 1.0 / (l.constant + l.linear * d + l.quadratic * d * d);
    return atten * intensity *
           blinnPhong(lightDir, norm, viewDir,
                      l.ambient, l.diffuse, l.specular, diffTex, specTex);
}

// ---------------------------------------------------------------------------
void main()
{
    vec3 norm    = normalize(vNormal);
    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec3 diffTex = texture(uMaterial.diffuse,  vUV).rgb;
    vec3 specTex = texture(uMaterial.specular, vUV).rgb;

    // --- Directional light (shadowed) ---
    vec3  dirL    = normalize(-uDirLight.direction);
    float shadow  = calcShadow(vFragPosLightSpace, norm, dirL);
    vec3  dirFull = calcDirLight(uDirLight, norm, viewDir, diffTex, specTex);
    // Shadow dims diffuse + specular; ambient always contributes so objects in
    // shadow don't go completely black.
    vec3  dirAmb  = uDirLight.ambient * diffTex;
    vec3  result  = dirAmb + (dirFull - dirAmb) * (1.0 - shadow);

    // --- Point lights (unshadowed) ---
    for (int i = 0; i < uNumPointLights; ++i)
        result += calcPointLight(uPointLights[i], norm, vFragPos, viewDir, diffTex, specTex);

    // --- Spot light / flashlight (unshadowed) ---
    if (uUseSpotLight)
        result += calcSpotLight(uSpotLight, norm, vFragPos, viewDir, diffTex, specTex);

    FragColor = vec4(result, 1.0);
}
