#include "Renderer/ShadowPass.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>

ShadowPass::ShadowPass(int width, int height)
    : m_width(width), m_height(height)
{
    // Depth texture — no colour attachment; we only store depth.
    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    // NEAREST filter: we do PCF manually in the shader, so we want the raw
    // depth value, not a blurred average.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Clamp to border (depth = 1.0) so fragments outside the shadow frustum
    // sample "max depth" and are therefore never considered in shadow.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float border[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    // FBO setup — attach only a depth buffer.
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, m_depthTexture, 0);
    glDrawBuffer(GL_NONE); // no colour attachment
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::fprintf(stderr, "[ShadowPass] FBO incomplete\n");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

ShadowPass::~ShadowPass()
{
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteTextures(1, &m_depthTexture);
}

void ShadowPass::beginFrame(const glm::vec3& lightDir,
                             const glm::vec3& sceneCenter,
                             float            sceneRadius)
{
    const glm::vec3 dir = glm::normalize(lightDir);

    // Choose an up vector that won't be collinear with the light direction.
    const glm::vec3 up = (glm::abs(dir.y) < 0.99f)
                       ? glm::vec3(0, 1, 0)
                       : glm::vec3(0, 0, 1);

    // Position the virtual "light camera" far back along the light direction.
    const glm::vec3 lightPos  = sceneCenter - dir * sceneRadius * 2.5f;
    const glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, up);

    // Orthographic projection covers the scene bounding sphere.
    // Near/far are generous to avoid clipping shadow casters outside the view.
    const glm::mat4 lightProj = glm::ortho(
        -sceneRadius, sceneRadius,
        -sceneRadius, sceneRadius,
        0.5f, sceneRadius * 6.0f);

    m_lightSpaceMatrix = lightProj * lightView;

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
}

void ShadowPass::endFrame(int viewportW, int viewportH)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, viewportW, viewportH);
}

void ShadowPass::bindDepthMap(unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
}
