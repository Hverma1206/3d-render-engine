#version 410 core

in vec2 vUV;
in vec3 vNormal;    // unused until Phase 8 lighting; declared to match vert out
in vec3 vFragPos;   // unused until Phase 8 lighting

out vec4 FragColor;

uniform sampler2D uTexture; // texture image unit 0

void main()
{
    FragColor = texture(uTexture, vUV);
}
