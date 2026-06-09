#pragma once

#include <glm/glm.hpp>

// A first-person "fly" camera. Owns its position + orientation (yaw/pitch) and
// produces the view and projection matrices the renderer feeds to shaders.
//
// Convention: right-handed, world-up = +Y, default facing -Z (yaw = -90 deg),
// matching OpenGL's clip-space / glm::lookAt expectations.
class Camera
{
public:
    enum class Movement { Forward, Backward, Left, Right, Up, Down };

    explicit Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f));

    glm::mat4 viewMatrix() const;                  // world -> view (eye) space
    glm::mat4 projectionMatrix(float aspect) const; // view -> clip space

    void move(Movement direction, float dt); // dt-scaled translation
    void look(float deltaX, float deltaY);   // mouse delta in pixels

    const glm::vec3& position() const { return m_position; }
    const glm::vec3& front()    const { return m_front; }
    float fov() const { return m_fov; }

private:
    void updateBasis(); // recompute front/right/up from yaw + pitch

    glm::vec3 m_position;
    glm::vec3 m_front{0.0f, 0.0f, -1.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};
    glm::vec3 m_right{1.0f, 0.0f, 0.0f};
    glm::vec3 m_worldUp{0.0f, 1.0f, 0.0f};

    float m_yaw   = -90.0f; // degrees; -90 so we start looking down -Z
    float m_pitch =   0.0f; // degrees; clamped to (-89, 89)

    float m_moveSpeed   = 3.0f;  // world units / second
    float m_sensitivity = 0.1f;  // degrees / pixel of mouse motion
    float m_fov         = 45.0f; // vertical field of view, degrees

    float m_near = 0.1f;
    float m_far  = 100.0f;
};
