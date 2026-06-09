#pragma once

#include "Core/Window.h"
#include "Scene/Camera.h"

#include <memory>

class Shader;  // forward-declared; full types only needed in the .cpp
class Texture;
class Input;

// Top-level owner of the window and the main render loop. As the project grows
// this is where the renderer, scene, camera, and UI will be wired together.
class Application
{
public:
    Application();
    ~Application();
    int run();

private:
    void initCube();              // Phase 5 geometry (extracted into Mesh in Phase 6)
    void processInput(float dt);

    Window m_window;
    Camera m_camera;
    std::unique_ptr<Input> m_input;

    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_ebo = 0;
    std::unique_ptr<Shader>  m_shader;
    std::unique_ptr<Texture> m_texture;

    float m_lastFrame = 0.0f;
};
