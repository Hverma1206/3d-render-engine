#version 410 core

// Empty fragment shader: depth writes happen automatically from gl_FragCoord.z.
// The FBO has no colour attachment, so we must output nothing.
void main() {}
