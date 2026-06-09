#pragma once

#include "Core/Window.h"

// Top-level owner of the window and the main render loop. As the project grows
// this is where the renderer, scene, camera, and UI will be wired together.
class Application
{
public:
    Application();
    ~Application();
    int run();

private:
    // Phase 2: build the triangle's GPU buffers + a minimal inline shader.
    // These raw GL handles get extracted into a Shader class (Phase 3) and a
    // Mesh class (Phase 6); they live here for now to keep Phase 2 focused.
    void initTriangle();

    Window m_window;

    unsigned int m_vao     = 0; // vertex array object (the format/binding state)
    unsigned int m_vbo     = 0; // vertex buffer object (the vertex data)
    unsigned int m_ebo     = 0; // element buffer object (the indices)
    unsigned int m_program = 0; // linked shader program
};
