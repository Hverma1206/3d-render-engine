#include "Scene/Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

Camera::Camera(glm::vec3 position)
    : m_position(position)
{
    updateBasis();
}

glm::mat4 Camera::viewMatrix() const
{
    // lookAt builds the world->view transform from eye, target, up.
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::projectionMatrix(float aspect) const
{
    return glm::perspective(glm::radians(m_fov), aspect, m_near, m_far);
}

void Camera::move(Movement direction, float dt)
{
    const float velocity = m_moveSpeed * dt; // frame-rate independent
    switch (direction)
    {
        case Movement::Forward:  m_position += m_front   * velocity; break;
        case Movement::Backward: m_position -= m_front   * velocity; break;
        case Movement::Left:     m_position -= m_right   * velocity; break;
        case Movement::Right:    m_position += m_right   * velocity; break;
        case Movement::Up:       m_position += m_worldUp * velocity; break;
        case Movement::Down:     m_position -= m_worldUp * velocity; break;
    }
}

void Camera::look(float deltaX, float deltaY)
{
    m_yaw   += deltaX * m_sensitivity;
    m_pitch -= deltaY * m_sensitivity; // screen-Y grows downward -> invert

    // Clamp pitch so the view never flips over the poles (gimbal flip).
    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

    updateBasis();
}

void Camera::updateBasis()
{
    // Spherical -> Cartesian: derive the forward vector from yaw + pitch.
    const float yaw   = glm::radians(m_yaw);
    const float pitch = glm::radians(m_pitch);

    glm::vec3 f;
    f.x = std::cos(yaw) * std::cos(pitch);
    f.y = std::sin(pitch);
    f.z = std::sin(yaw) * std::cos(pitch);
    m_front = glm::normalize(f);

    // Re-orthogonalize the basis. right = front x worldUp, up = right x front.
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
}
