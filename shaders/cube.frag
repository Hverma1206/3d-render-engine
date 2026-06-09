#version 410 core

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uTexture; // bound to texture image unit 0

void main()
{
    FragColor = texture(uTexture, vUV);
}
