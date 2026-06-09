#include "Core/Application.h"

#include "Core/Input.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>
#include <string>

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

    initCube();
    m_input = std::make_unique<Input>(m_window.handle());
}

Application::~Application()
{
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
    // m_shader / m_texture release their GL objects automatically (RAII).
}

void Application::initCube()
{
    // 24 vertices (4 per face) so every face gets its own full 0..1 UV square.
    // Layout per vertex: [ position.xyz | uv.xy ] -> stride 5 floats.
    const float vertices[] = {
        // position             // uv
        // +Z (front)
        -0.5f, -0.5f,  0.5f,    0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,    1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,    1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,    0.0f, 1.0f,
        // -Z (back)
         0.5f, -0.5f, -0.5f,    0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,    1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,    1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,    0.0f, 1.0f,
        // -X (left)
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,    1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,    1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,    0.0f, 1.0f,
        // +X (right)
         0.5f, -0.5f,  0.5f,    0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,    1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,    1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,    0.0f, 1.0f,
        // +Y (top)
        -0.5f,  0.5f,  0.5f,    0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,    1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,    1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,    0.0f, 1.0f,
        // -Y (bottom)
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,    1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,    1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,    0.0f, 1.0f,
    };

    // Two triangles per face: (0,1,2) (2,3,0), offset by 4 per face.
    unsigned int indices[36];
    for (unsigned int f = 0; f < 6; ++f)
    {
        const unsigned int b = f * 4;
        const unsigned int o = f * 6;
        indices[o + 0] = b + 0; indices[o + 1] = b + 1; indices[o + 2] = b + 2;
        indices[o + 3] = b + 2; indices[o + 4] = b + 3; indices[o + 5] = b + 0;
    }

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    const GLsizei stride = 5 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    const std::string root = RS_PROJECT_ROOT;
    m_shader = std::make_unique<Shader>(root + "/shaders/cube.vert",
                                        root + "/shaders/cube.frag");
    if (!m_shader->valid())
        std::fprintf(stderr, "[Application] cube shader failed to build\n");

    // NOTE: this is a color map, so it *should* be sRGB. But we don't yet
    // re-encode linear->sRGB on output (that arrives with the HDR/gamma
    // pipeline in Phase 13), so loading it as sRGB now would look too dark.
    // Load linear for a correct-looking demo today; flip to srgb=true once the
    // gamma-correct output stage exists. Tell the sampler to read unit 0.
    m_texture = std::make_unique<Texture>(root + "/assets/textures/uv_grid.bmp",
                                          /*srgb=*/false);
    m_shader->setInt("uTexture", 0);
}

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

        const int fbw = m_window.framebufferWidth();
        const int fbh = m_window.framebufferHeight();
        const float aspect = fbh > 0 ? static_cast<float>(fbw) / fbh : 1.0f;

        glm::mat4 model = glm::rotate(glm::mat4(1.0f), now * 0.5f, glm::vec3(0.4f, 1.0f, 0.0f));
        m_shader->setMat4("uModel", model);
        m_shader->setMat4("uView", m_camera.viewMatrix());
        m_shader->setMat4("uProjection", m_camera.projectionMatrix(aspect));

        m_shader->use();
        m_texture->bind(0);
        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);

        if (!verified)
        {
            verified = true;
            unsigned char px[3] = { 0, 0, 0 };
            glReadPixels(fbw / 2, fbh / 2, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, px);
            std::printf("Center pixel RGB = (%d, %d, %d)  [clear was ~(18,33,38)]\n",
                        px[0], px[1], px[2]);
            std::printf("Texture loaded: %s (%dx%d)\n",
                        m_texture->valid() ? "yes" : "NO",
                        m_texture->width(), m_texture->height());
            std::printf("GL error after first frame: 0x%04X\n", glGetError());
            std::fflush(stdout);
        }

        m_window.swapBuffers();
    }
    return 0;
}
