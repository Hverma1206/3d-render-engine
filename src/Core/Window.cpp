#include "Core/Window.h"

#include <glad/glad.h>   // must precede the GLFW header
#include <GLFW/glfw3.h>

#include <cstdio>
#include <stdexcept>

static void glfwErrorCallback(int error, const char* description)
{
    std::fprintf(stderr, "[GLFW] error %d: %s\n", error, description);
}

Window::Window(int width, int height, std::string title)
{
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    // Request an OpenGL 4.1 Core context. On macOS FORWARD_COMPAT is
    // MANDATORY -- without it glfwCreateWindow() returns nullptr.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window / OpenGL 4.1 context");
    }

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);

    // Load OpenGL function pointers AFTER the context is current. Any GL call
    // before this point dereferences a null pointer and crashes.
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLAD (OpenGL loader)");
    }

    // Initialize the viewport from the real framebuffer size (Retina-aware).
    glfwGetFramebufferSize(m_window, &m_fbWidth, &m_fbHeight);
    glViewport(0, 0, m_fbWidth, m_fbHeight);

    glfwSwapInterval(1); // vsync on -> cap to display refresh rate
}

Window::~Window()
{
    if (m_window)
        glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(m_window);
}

void Window::pollEvents() const
{
    glfwPollEvents();

    // Minimal Phase-1 input: ESC requests close. A dedicated Input system
    // (and camera) arrives in Phase 4.
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void Window::swapBuffers() const
{
    glfwSwapBuffers(m_window);
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    self->m_fbWidth  = width;
    self->m_fbHeight = height;
    glViewport(0, 0, width, height);
}
