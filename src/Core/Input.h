#pragma once

struct GLFWwindow;

// Thin input helper over GLFW. Polls keyboard state on demand and tracks the
// per-frame mouse delta. Captures (hides + locks) the cursor for FPS-style
// mouse-look. Uses polling only -- no GLFW callbacks -- so it doesn't compete
// with Window for the single GLFW window-user-pointer slot.
class Input
{
public:
    explicit Input(GLFWwindow* window);

    void update();                     // call once per frame to refresh mouse delta
    bool isKeyDown(int glfwKey) const; // e.g. isKeyDown(GLFW_KEY_W)

    float mouseDeltaX() const { return m_dx; }
    float mouseDeltaY() const { return m_dy; }

private:
    GLFWwindow* m_window = nullptr;
    double m_lastX = 0.0, m_lastY = 0.0;
    float  m_dx = 0.0f, m_dy = 0.0f;
    bool   m_firstUpdate = true; // avoids a huge initial delta jump
};
