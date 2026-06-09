#version 410 core

// Canonical vertex layout — must match Vertex.h exactly.
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;      // transpose(inverse(mat3(uModel))) — precomputed on CPU
uniform mat4 uLightSpaceMatrix;  // world -> light clip space (for shadow mapping)

out vec3 vFragPos;           // world-space position
out vec3 vNormal;            // world-space normal (transformed by normal matrix)
out vec2 vUV;
out vec4 vFragPosLightSpace; // fragment position in light clip space

void main()
{
    vec4 worldPos    = uModel * vec4(aPos, 1.0);
    vFragPos         = worldPos.xyz;
    vNormal          = uNormalMatrix * aNormal;
    vUV              = aUV;
    vFragPosLightSpace = uLightSpaceMatrix * worldPos;
    gl_Position      = uProjection * uView * worldPos;
}
