#include "Graphics/Texture.h"

#include <glad/glad.h>

// stb_image is header-only: define the implementation in exactly one TU.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cstdio>
#include <utility>

Texture::Texture(const std::string& path, bool srgb, bool flipVertically)
{
    // OpenGL's texture origin is bottom-left; images are usually top-left.
    // Flipping on load makes UV (0,0) map to the bottom of the image.
    stbi_set_flip_vertically_on_load(flipVertically);

    unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &m_channels, 0);
    if (!data)
    {
        std::fprintf(stderr, "[Texture] failed to load '%s': %s\n",
                     path.c_str(), stbi_failure_reason());
        return; // m_id stays 0 -> valid() == false
    }

    // Choose GL formats from the channel count. internalFormat = how the GPU
    // stores it; dataFormat = how our CPU bytes are laid out.
    GLenum dataFormat     = GL_RGB;
    GLint  internalFormat = GL_RGB;
    if (m_channels == 1)      { dataFormat = GL_RED;  internalFormat = GL_RED; }
    else if (m_channels == 3) { dataFormat = GL_RGB;  internalFormat = srgb ? GL_SRGB8        : GL_RGB; }
    else if (m_channels == 4) { dataFormat = GL_RGBA; internalFormat = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA; }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    // Tightly-packed rows (3-channel data isn't always 4-byte aligned).
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0,
                 dataFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Trilinear (mipmapped) minification, linear magnification.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
}

Texture::~Texture()
{
    glDeleteTextures(1, &m_id); // glDeleteTextures on 0 is a safe no-op
}

Texture::Texture(Texture&& other) noexcept
    : m_id(other.m_id), m_width(other.m_width),
      m_height(other.m_height), m_channels(other.m_channels)
{
    other.m_id = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other)
    {
        glDeleteTextures(1, &m_id);
        m_id       = other.m_id;
        m_width    = other.m_width;
        m_height   = other.m_height;
        m_channels = other.m_channels;
        other.m_id = 0;
    }
    return *this;
}

void Texture::bind(unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_id);
}
