#include "Core/Application.h"

#include "Core/Input.h"
#include "Graphics/Mesh.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture.h"
#include "Graphics/Vertex.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Cube builder
// ---------------------------------------------------------------------------
// Returns a unit cube as a Mesh. 24 vertices (4 per face) so every face has
// its own clean 0..1 UV square and outward-facing normal for lighting.
std::unique_ptr<Mesh> Application::buildCube()
{
    // Helper: build one face's 4 vertices given the face normal and a 2D
    // parametric origin corner, U-axis, and V-axis in 3D.
    // This avoids 24 lines of copy-paste.
    auto face = [](glm::vec3 n, glm::vec3 o, glm::vec3 u, glm::vec3 v,
                   std::vector<Vertex>& verts, std::vector<unsigned int>& idx)
    {
        const unsigned int base = static_cast<unsigned int>(verts.size());
        verts.push_back({ o,         n, {0,0} });
        verts.push_back({ o + u,     n, {1,0} });
        verts.push_back({ o + u + v, n, {1,1} });
        verts.push_back({ o + v,     n, {0,1} });
        // CCW winding viewed from outside (i.e. from direction of normal)
        idx.insert(idx.end(), { base+0, base+1, base+2,
                                 base+2, base+3, base+0 });
    };

    std::vector<Vertex>       verts;
    std::vector<unsigned int> indices;
    verts.reserve(24);
    indices.reserve(36);

    const glm::vec3 h{0.5f, 0.5f, 0.5f}; // half-extent

    // +Z front
    face({ 0,0, 1}, {-h.x,-h.y, h.z}, { 1,0,0}, {0,1,0}, verts, indices);
    // -Z back
    face({ 0,0,-1}, { h.x,-h.y,-h.z}, {-1,0,0}, {0,1,0}, verts, indices);
    // -X left
    face({-1,0, 0}, {-h.x,-h.y,-h.z}, {0,0, 1}, {0,1,0}, verts, indices);
    // +X right
    face({ 1,0, 0}, { h.x,-h.y, h.z}, {0,0,-1}, {0,1,0}, verts, indices);
    // +Y top
    face({ 0,1, 0}, {-h.x, h.y, h.z}, { 1,0,0}, {0,0,-1}, verts, indices);
    // -Y bottom
    face({ 0,-1,0}, {-h.x,-h.y,-h.z}, { 1,0,0}, {0,0, 1}, verts, indices);

    return std::make_unique<Mesh>(std::move(verts), std::move(indices));
}

// ---------------------------------------------------------------------------
// Application
// ---------------------------------------------------------------------------
Application::Application()
    : m_window(1280, 720, "Real-Time Rendering Sandbox")
{
    std::printf("OpenGL Vendor   : %s\n", glGetString(GL_VENDOR));
    std::printf("OpenGL Renderer : %s\n", glGetString(GL_RENDERER));
    std::printf("OpenGL Version  : %s\n", glGetString(GL_VERSION));
    std::printf("GLSL Version    : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    std::printf("Controls: WASD move, mouse look, SPACE/LSHIFT up-down, ESC quit\n");
    std::fflush(stdout);

    glEnable(GL_DEPTH_TEST);

    const std::string root = RS_PROJECT_ROOT;

    m_mesh    = buildCube();
    m_shader  = std::make_unique<Shader>(root + "/shaders/cube.vert",
                                         root + "/shaders/cube.frag");
    m_texture = std::make_unique<Texture>(root + "/assets/textures/uv_grid.bmp",
                                          /*srgb=*/false);
    m_shader->setInt("uTexture", 0);
    m_input   = std::make_unique<Input>(m_window.handle());
}

Application::~Application() = default; // all members clean up via RAII

void Application::processInput(float dt)
{
    m_input->update();

    if (m_input->isKeyDown(GLFW_KEY_W))          m_camera.move(Camera::Movement::Forward,  dt);
    if (m_input->isKeyDown(GLFW_KEY_S))          m_camera.move(Camera::Movement::Backward, dt);
    if (m_input->isKeyDown(GLFW_KEY_A))          m_camera.move(Camera::Movement::Left,     dt);
    if (m_input->isKeyDown(GLFW_KEY_D))          m_camera.move(Camera::Movement::Right,    dt);
    if (m_input->isKeyDown(GLFW_KEY_SPACE))      m_camera.move(Camera::Movement::Up,       dt);
    if (m_input->isKeyDown(GLFW_KEY_LEFT_SHIFT)) m_camera.move(Camera::Movement::Down,     dt);

    m_camera.look(m_input->mouseDeltaX(), m_input->mouseDeltaY());
}

int Application::run()
{
    bool verified = false;

    while (!m_window.shouldClose())
    {
        const float now = static_cast<float>(glfwGetTime());
        const float dt  = now - m_lastFrame;
        m_lastFrame = now;

        m_window.pollEvents();
        processInput(dt);

        glClearColor(0.07f, 0.13f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspect = m_window.framebufferHeight() > 0
            ? static_cast<float>(m_window.framebufferWidth()) / m_window.framebufferHeight()
            : 1.0f;

        glm::mat4 model = glm::rotate(glm::mat4(1.0f),
                                      now * 0.5f, glm::vec3(0.4f, 1.0f, 0.0f));
        m_shader->setMat4("uModel",      model);
        m_shader->setMat4("uView",       m_camera.viewMatrix());
        m_shader->setMat4("uProjection", m_camera.projectionMatrix(aspect));

        m_shader->use();
        m_texture->bind(0);
        m_mesh->draw();

        if (!verified)
        {
            verified = true;
            const int w = m_window.framebufferWidth();
            const int h = m_window.framebufferHeight();
            unsigned char px[3] = {};
            glReadPixels(w / 2, h / 2, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, px);
            std::printf("Center pixel RGB = (%d, %d, %d)\n", px[0], px[1], px[2]);
            std::printf("Mesh: %u vertices, %u indices\n",
                        m_mesh->vertexCount(), m_mesh->indexCount());
            std::printf("GL error after first frame: 0x%04X\n", glGetError());
            std::fflush(stdout);
        }

        m_window.swapBuffers();
    }
    return 0;
}
