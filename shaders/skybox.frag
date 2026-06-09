#version 410 core
// Phase 16 — Skybox fragment shader.
// Samples the cubemap and adds a procedural sky-to-ground gradient so the
// solid-colour faces blend into a convincing horizon.
in vec3 vDir;
out vec4 fragColor;

uniform samplerCube uSkybox;
uniform float uTime = 0.0;

void main()
{
    vec3 dir = normalize(vDir);
    vec4 base = texture(uSkybox, dir);

    // Procedural gradient: zenith → horizon → ground
    float y = dir.y;
    vec3 zenith  = vec3(0.10, 0.28, 0.70);
    vec3 horizon = vec3(0.56, 0.76, 0.98);
    vec3 ground  = vec3(0.14, 0.11, 0.07);
    vec3 sky = (y >= 0.0)
        ? mix(horizon, zenith, y * y)
        : mix(ground, horizon, clamp(y + 1.0, 0.0, 1.0));

    // Sun disc (cheap: directional highlight toward (1,-0.4,-0.8))
    vec3 sunDir = normalize(vec3(1.0, -0.4, -0.8));
    float sun = pow(max(dot(dir, -sunDir), 0.0), 512.0);
    sky += vec3(1.0, 0.95, 0.8) * sun * 3.0;

    fragColor = vec4(sky * base.rgb * 1.8, 1.0);
}
