#version 410 core

// Shadow-pass vertex shader: only position is needed.
// The light-space transform replaces the camera's view+projection.
layout (location = 0) in vec3 aPos;

uniform mat4 uModel;
uniform mat4 uLightSpaceMatrix;

void main()
{
    gl_Position = uLightSpaceMatrix * uModel * vec4(aPos, 1.0);
}
