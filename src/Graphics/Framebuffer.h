#pragma once
#include <glad/glad.h>
#include <vector>

// Phase 11 — Generic FBO wrapper.
// Owns N color texture attachments plus an optional depth (texture or RBO).
// resize() tears down and re-creates every attachment at new dimensions.
class Framebuffer
{
public:
    Framebuffer(int w, int h,
                std::vector<GLenum> colorFormats,
                bool hasDepth   = true,
                bool depthAsTex = false);
    ~Framebuffer();
    Framebuffer(Framebuffer&&) noexcept;
    Framebuffer& operator=(Framebuffer&&) noexcept;
    Framebuffer(const Framebuffer&)            = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void bind()          const;
    static void unbind();
    void resize(int w, int h);

    GLuint colorTex(int index = 0) const;
    GLuint depthTex()              const;
    GLuint fboId()                 const { return m_fbo; }
    int    width()                 const { return m_w;  }
    int    height()                const { return m_h;  }
    bool   valid()                 const { return m_fbo != 0; }

private:
    void create();
    void destroy();

    int  m_w, m_h;
    bool m_hasDepth, m_depthAsTex;
    std::vector<GLenum> m_colorFmts;

    GLuint m_fbo = 0;
    std::vector<GLuint> m_colorTex;
    GLuint m_depth = 0;   // texture OR renderbuffer, depending on m_depthAsTex
};
