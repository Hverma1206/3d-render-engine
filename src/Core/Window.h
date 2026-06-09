#pragma once

#include <string>

// Forward declaration keeps the GLFW header out of this interface; only the
// .cpp needs the full type. Faster compiles, fewer transitive includes.
struct GLFWwindow;

// RAII wrapper around a GLFW window and its OpenGL 4.1 Core context.
// Construction initializes GLFW, creates the window/context, and loads GL
// function pointers via GLAD. Destruction tears everything down.
class Window
{
public:
    Window(int width, int height, std::string title);
    ~Window();

    // Single owning handle for the app lifetime: non-copyable, non-movable.
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void pollEvents() const;
    void swapBuffers() const;

    // Framebuffer size in *pixels* (Retina-aware: can be 2x the window size).
    // Always drive glViewport / projection aspect from these, not window size.
    int framebufferWidth()  const { return m_fbWidth; }
    int framebufferHeight() const { return m_fbHeight; }

    GLFWwindow* handle() const { return m_window; }

private:
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow* m_window = nullptr;
    int m_fbWidth  = 0;
    int m_fbHeight = 0;
};
