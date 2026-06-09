#include "Graphics/Framebuffer.h"
#include <cstdio>

Framebuffer::Framebuffer(int w, int h,
                         std::vector<GLenum> colorFormats,
                         bool hasDepth, bool depthAsTex)
    : m_w(w), m_h(h)
    , m_hasDepth(hasDepth), m_depthAsTex(depthAsTex)
    , m_colorFmts(std::move(colorFormats))
{
    create();
}

Framebuffer::~Framebuffer() { destroy(); }

Framebuffer::Framebuffer(Framebuffer&& o) noexcept
    : m_w(o.m_w), m_h(o.m_h)
    , m_hasDepth(o.m_hasDepth), m_depthAsTex(o.m_depthAsTex)
    , m_colorFmts(std::move(o.m_colorFmts))
    , m_fbo(o.m_fbo)
    , m_colorTex(std::move(o.m_colorTex))
    , m_depth(o.m_depth)
{
    o.m_fbo = o.m_depth = 0;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& o) noexcept
{
    if (this != &o) {
        destroy();
        m_w = o.m_w;  m_h = o.m_h;
        m_hasDepth = o.m_hasDepth;  m_depthAsTex = o.m_depthAsTex;
        m_colorFmts = std::move(o.m_colorFmts);
        m_fbo       = o.m_fbo;
        m_colorTex  = std::move(o.m_colorTex);
        m_depth     = o.m_depth;
        o.m_fbo = o.m_depth = 0;
    }
    return *this;
}

void Framebuffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_w, m_h);
}

void Framebuffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resize(int w, int h)
{
    if (w == m_w && h == m_h) return;
    m_w = w;  m_h = h;
    destroy();
    create();
}

GLuint Framebuffer::colorTex(int i) const
{
    return (i >= 0 && i < (int)m_colorTex.size()) ? m_colorTex[i] : 0;
}

GLuint Framebuffer::depthTex() const
{
    return m_depthAsTex ? m_depth : 0;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------
static GLenum baseFormatOf(GLenum internal)
{
    switch (internal) {
    case GL_RGBA16F: case GL_RGBA32F: case GL_RGBA8: return GL_RGBA;
    case GL_RGB16F:  case GL_RGB32F:  case GL_RGB8:  return GL_RGB;
    case GL_RG16F:   case GL_RG32F:   case GL_RG8:   return GL_RG;
    case GL_R16F:    case GL_R32F:    case GL_R8:    return GL_RED;
    default:                                          return GL_RGBA;
    }
}

static GLenum dataTypeOf(GLenum internal)
{
    switch (internal) {
    case GL_RGBA16F: case GL_RGB16F: case GL_RG16F: case GL_R16F:
    case GL_RGBA32F: case GL_RGB32F: case GL_RG32F: case GL_R32F:
        return GL_FLOAT;
    default:
        return GL_UNSIGNED_BYTE;
    }
}

void Framebuffer::create()
{
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // --- colour attachments ---
    m_colorTex.resize(m_colorFmts.size());
    if (!m_colorTex.empty())
        glGenTextures((GLsizei)m_colorTex.size(), m_colorTex.data());

    std::vector<GLenum> drawBufs;
    for (int i = 0; i < (int)m_colorFmts.size(); ++i)
    {
        GLenum ifmt   = m_colorFmts[i];
        GLenum bfmt   = baseFormatOf(ifmt);
        GLenum dtype  = dataTypeOf(ifmt);
        glBindTexture(GL_TEXTURE_2D, m_colorTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, ifmt, m_w, m_h, 0, bfmt, dtype, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0 + i,
                               GL_TEXTURE_2D, m_colorTex[i], 0);
        drawBufs.push_back(GL_COLOR_ATTACHMENT0 + i);
    }

    if (!drawBufs.empty())
        glDrawBuffers((GLsizei)drawBufs.size(), drawBufs.data());
    else
        glDrawBuffer(GL_NONE);

    // --- depth attachment ---
    if (m_hasDepth)
    {
        if (m_depthAsTex) {
            glGenTextures(1, &m_depth);
            glBindTexture(GL_TEXTURE_2D, m_depth);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
                         m_w, m_h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                   GL_TEXTURE_2D, m_depth, 0);
        } else {
            glGenRenderbuffers(1, &m_depth);
            glBindRenderbuffer(GL_RENDERBUFFER, m_depth);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_w, m_h);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                      GL_DEPTH_STENCIL_ATTACHMENT,
                                      GL_RENDERBUFFER, m_depth);
        }
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        std::fprintf(stderr, "[Framebuffer] incomplete: 0x%04X (w=%d h=%d)\n",
                     status, m_w, m_h);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::destroy()
{
    if (!m_colorTex.empty()) {
        glDeleteTextures((GLsizei)m_colorTex.size(), m_colorTex.data());
        m_colorTex.clear();
    }
    if (m_depth) {
        if (m_depthAsTex) glDeleteTextures(1, &m_depth);
        else              glDeleteRenderbuffers(1, &m_depth);
        m_depth = 0;
    }
    if (m_fbo) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
}
