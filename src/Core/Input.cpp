#include "Core/Input.h"

#include <GLFW/glfw3.h>

Input::Input(GLFWwindow* window)
    : m_window(window)
{
    // FPS-style capture: hide the cursor and lock it to the window so the user
    // can look around continuously without the pointer leaving the window.
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Input::update()
{
    double x, y;
    glfwGetCursorPos(m_window, &x, &y);

    if (m_firstUpdate)
    {
        // Seed last-position so the first frame reports zero movement instead
        // of a large jump from the window's initial cursor location.
        m_lastX = x;
        m_lastY = y;
        m_firstUpdate = false;
    }

    m_dx = static_cast<float>(x - m_lastX);
    m_dy = static_cast<float>(y - m_lastY);
    m_lastX = x;
    m_lastY = y;
}

bool Input::isKeyDown(int glfwKey) const
{
    return glfwGetKey(m_window, glfwKey) == GLFW_PRESS;
}
