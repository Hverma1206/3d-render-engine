#include "Graphics/Texture.h"

#include <glad/glad.h>

// stb_image is header-only: define the implementation in exactly one TU.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cstdio>

Texture::Texture(const std::string& path, bool srgb, bool flipVertically)
{
    stbi_set_flip_vertically_on_load(flipVertically);
    unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &m_channels, 0);
    if (!data)
    {
        std::fprintf(stderr, "[Texture] failed to load '%s': %s\n",
                     path.c_str(), stbi_failure_reason());
        return;
    }

    GLenum dataFormat     = GL_RGB;
    GLint  internalFormat = GL_RGB;
    if (m_channels == 1)      { dataFormat = GL_RED;  internalFormat = GL_RED; }
    else if (m_channels == 3) { dataFormat = GL_RGB;  internalFormat = srgb ? GL_SRGB8        : GL_RGB; }
    else if (m_channels == 4) { dataFormat = GL_RGBA; internalFormat = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA; }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0,
                 dataFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
}

Texture::~Texture()
{
    glDeleteTextures(1, &m_id);
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

// Static factories: private default ctor is accessible inside static members.
Texture Texture::createWhite()
{
    Texture t;
    t.m_width = t.m_height = 1; t.m_channels = 3;
    glGenTextures(1, &t.m_id);
    glBindTexture(GL_TEXTURE_2D, t.m_id);
    unsigned char px[3] = {255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, px);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return t;
}

Texture Texture::createGrey()
{
    Texture t;
    t.m_width = t.m_height = 1; t.m_channels = 3;
    glGenTextures(1, &t.m_id);
    glBindTexture(GL_TEXTURE_2D, t.m_id);
    unsigned char px[3] = {128, 128, 128};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, px);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return t;
}
