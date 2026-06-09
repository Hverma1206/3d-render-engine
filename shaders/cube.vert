#version 410 core

// Canonical layout -- matches Vertex.h and every geometry shader in this project.
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec2  vUV;
out vec3  vNormal;   // passed through now; used for lighting from Phase 8
out vec3  vFragPos;  // world-space position, needed for lighting

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos  = worldPos.xyz;
    // Normal matrix: inverse transpose of the model matrix upper-left 3x3.
    // Keeps normals perpendicular to the surface after non-uniform scaling.
    vNormal   = mat3(transpose(inverse(uModel))) * aNormal;
    vUV       = aUV;
    gl_Position = uProjection * uView * worldPos;
}
