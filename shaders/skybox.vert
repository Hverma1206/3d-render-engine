#version 410 core
// Phase 16 — Skybox vertex shader.
// Removes translation from the view matrix so the cube stays centred on the
// camera.  Sets z = w so the cube maps to the far plane after division (z=1).
layout(location = 0) in vec3 aPos;

out vec3 vDir;

uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    vDir = aPos;
    // Strip translation: mat4(mat3(view)) zeroes the last column's xyz
    vec4 pos = uProjection * mat4(mat3(uView)) * vec4(aPos, 1.0);
    // xyww: z/w = w/w = 1.0 → sits on the far plane; needs GL_LEQUAL depth test
    gl_Position = pos.xyww;
}
