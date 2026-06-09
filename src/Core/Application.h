#pragma once

#include "Core/Window.h"
#include "Scene/Camera.h"

#include <memory>

class Shader;
class Texture;
class Mesh;
class Input;

// Top-level owner of the window and the main render loop.
class Application
{
public:
    Application();
    ~Application();
    int run();

private:
    static std::unique_ptr<Mesh> buildCube();  // Phase 6: factory, returns a Mesh
    void processInput(float dt);

    Window m_window;
    Camera m_camera;
    std::unique_ptr<Input>   m_input;
    std::unique_ptr<Mesh>    m_mesh;
    std::unique_ptr<Shader>  m_shader;
    std::unique_ptr<Texture> m_texture;

    float m_lastFrame = 0.0f;
};
