#include "Core/Application.h"

#include <glad/glad.h>

#include <cstdio>

// ---------------------------------------------------------------------------
// Phase 2 inline shaders.
// Kept as string literals here; Phase 3 moves these into .vert/.frag files
// loaded by a reusable Shader class with proper error handling.
// ---------------------------------------------------------------------------
static const char* kVertexShaderSrc = R"(#version 410 core
layout (location = 0) in vec3 aPos;     // attribute slot 0: position
layout (location = 1) in vec3 aColor;   // attribute slot 1: per-vertex color

out vec3 vColor;                        // passed to the fragment shader (interpolated)

void main()
{
    vColor      = aColor;
    gl_Position = vec4(aPos, 1.0);      // already in clip space (no matrices yet)
}
)";

static const char* kFragmentShaderSrc = R"(#version 410 core
in  vec3 vColor;                        // interpolated across the triangle
out vec4 FragColor;                     // the single color attachment (the screen)

void main()
{
    FragColor = vec4(vColor, 1.0);
}
)";

// ---------------------------------------------------------------------------
// Local shader compile/link helpers (Phase 2 only; replaced by Shader class).
// ---------------------------------------------------------------------------
static unsigned int compileShader(GLenum type, const char* src)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[Shader] %s compile error:\n%s\n",
                     type == GL_VERTEX_SHADER ? "vertex" : "fragment", log);
    }
    return shader;
}

static unsigned int linkProgram(unsigned int vs, unsigned int fs)
{
    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[Shader] program link error:\n%s\n", log);
    }

    // Once linked, the individual shader objects are no longer needed.
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
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
    std::fflush(stdout);

    initTriangle();
}

Application::~Application()
{
    // Release GPU resources. (Phase 6's Mesh / Phase 3's Shader will do this
    // automatically via RAII; for now we clean up explicitly.)
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
    glDeleteProgram(m_program);
}

void Application::initTriangle()
{
    // Interleaved vertex data: [ position.xyz | color.rgb ] per vertex.
    // Positions are already in Normalized Device Coordinates (-1..1).
    const float vertices[] = {
        // position            // color
        -0.5f, -0.5f, 0.0f,    1.0f, 0.0f, 0.0f,  // 0: bottom-left  (red)
         0.5f, -0.5f, 0.0f,    0.0f, 1.0f, 0.0f,  // 1: bottom-right (green)
         0.0f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f,  // 2: top          (blue)
    };
    const unsigned int indices[] = { 0, 1, 2 };

    // 1) VAO captures the vertex format + which VBO/EBO are bound. Bind first.
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // 2) VBO: upload the vertex bytes to GPU memory once (STATIC = never changes).
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 3) EBO: upload the indices. While the VAO is bound, the EBO binding is
    //    recorded into it, so we don't re-bind the EBO at draw time.
    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // 4) Describe the memory layout so the GPU can fetch each attribute.
    const GLsizei stride = 6 * sizeof(float);
    // location 0: position, 3 floats, offset 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    // location 1: color, 3 floats, offset 3 floats
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 5) Unbind the VAO to avoid accidental state changes elsewhere.
    glBindVertexArray(0);

    // 6) Build the shader program.
    unsigned int vs = compileShader(GL_VERTEX_SHADER,   kVertexShaderSrc);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, kFragmentShaderSrc);
    m_program = linkProgram(vs, fs);
}

int Application::run()
{
    bool verified = false;

    while (!m_window.shouldClose())
    {
        m_window.pollEvents();

        glClearColor(0.07f, 0.13f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw: bind the program + VAO, then issue an indexed draw call.
        glUseProgram(m_program);
        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);

        // One-time headless verification: the screen-center pixel lies inside
        // the triangle, so it must differ from the clear color.
        if (!verified)
        {
            verified = true;
            const int w = m_window.framebufferWidth();
            const int h = m_window.framebufferHeight();
            unsigned char px[3] = { 0, 0, 0 };
            glReadPixels(w / 2, h / 2, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, px);
            std::printf("Center pixel RGB = (%d, %d, %d)  [clear was ~(18,33,38)]\n",
                        px[0], px[1], px[2]);
            std::printf("GL error after first frame: 0x%04X\n", glGetError());
            std::fflush(stdout);
        }

        m_window.swapBuffers();
    }
    return 0;
}
