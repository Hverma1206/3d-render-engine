#pragma once

#include <glm/glm.hpp>

// Manages a depth-only framebuffer object (FBO) for directional-light shadow
// mapping. The FBO has a single GL_DEPTH_COMPONENT texture attachment and no
// colour attachment — we only need depth values.
//
// Typical per-frame usage:
//   shadow.beginFrame(lightDir, sceneCenter, sceneRadius);
//       depthShader.use();
//       // draw all opaque geometry with uModel + uLightSpaceMatrix set
//   shadow.endFrame(fbWidth, fbHeight);
//   // ... lighting pass: shadow.bindDepthMap(unit)
class ShadowPass
{
public:
    explicit ShadowPass(int width = 2048, int height = 2048);
    ~ShadowPass();

    ShadowPass(const ShadowPass&)            = delete;
    ShadowPass& operator=(const ShadowPass&) = delete;

    // Compute the light-space matrix, bind the FBO, set the viewport,
    // and clear the depth buffer. Must be followed by endFrame().
    void beginFrame(const glm::vec3& lightDir,
                    const glm::vec3& sceneCenter = glm::vec3(0.0f),
                    float            sceneRadius  = 8.0f);

    // Unbind the FBO and restore the viewport to the screen dimensions.
    void endFrame(int viewportW, int viewportH);

    // Bind the depth texture to a texture image unit for the lighting pass.
    void bindDepthMap(unsigned int unit) const;

    const glm::mat4& lightSpaceMatrix() const { return m_lightSpaceMatrix; }
    int width()  const { return m_width;  }
    int height() const { return m_height; }

private:
    unsigned int m_fbo          = 0;
    unsigned int m_depthTexture = 0;
    int          m_width, m_height;
    glm::mat4    m_lightSpaceMatrix{1.0f};
};
