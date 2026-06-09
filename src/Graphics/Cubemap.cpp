#include "Graphics/Cubemap.h"
#include <cstdio>
#include <vector>
#include <cmath>
#include <algorithm>

// stb_image — implementation is compiled in Texture.cpp; only declarations here.
#include "stb_image.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void setCommonCubemapParams()
{
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

// ---------------------------------------------------------------------------
// Procedural sky cubemap
// ---------------------------------------------------------------------------
// For each face we fill a SIZE×SIZE RGB texture.
// The direction vector for pixel (u,v) on each face maps to a world Y coordinate,
// which drives the sky-to-ground gradient.
static float facePixelY(int face, float u, float v)
{
    // u,v in [-1, 1]
    // Maps cubemap face+UV to world-space direction Y component
    switch (face) {
    case 0: return  u;  // +X: dir = (1, v, -u), y=v  — but we want a rough estimate
    case 1: return  u;  // -X
    case 2: return  1.0f; // +Y: always sky
    case 3: return -1.0f; // -Y: always ground
    case 4: return  v;  // +Z
    case 5: return  v;  // -Z
    default: return 0.0f;
    }
}

Cubemap Cubemap::createSky()
{
    Cubemap c;
    glGenTextures(1, &c.m_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, c.m_id);

    constexpr int SIZE = 64;
    std::vector<unsigned char> pixels(SIZE * SIZE * 3);

    // zenith (top), horizon, ground colours
    const float zenith[3]   = {0.10f, 0.28f, 0.70f};
    const float horizon[3]  = {0.56f, 0.76f, 0.98f};
    const float ground[3]   = {0.14f, 0.11f, 0.07f};

    for (int face = 0; face < 6; ++face)
    {
        for (int y = 0; y < SIZE; ++y)
        {
            for (int x = 0; x < SIZE; ++x)
            {
                float u = (x + 0.5f) / SIZE * 2.0f - 1.0f;
                float v = (y + 0.5f) / SIZE * 2.0f - 1.0f;
                float worldY = facePixelY(face, u, v);

                // Smooth blend: ground→horizon→zenith
                float t = std::max(worldY, -1.0f);
                t = std::min(t, 1.0f);

                float r, g, b;
                if (t >= 0.0f) {
                    r = horizon[0] + (zenith[0] - horizon[0]) * t * t;
                    g = horizon[1] + (zenith[1] - horizon[1]) * t * t;
                    b = horizon[2] + (zenith[2] - horizon[2]) * t * t;
                } else {
                    float s = (t + 1.0f);  // 0..1
                    r = ground[0] + (horizon[0] - ground[0]) * s;
                    g = ground[1] + (horizon[1] - ground[1]) * s;
                    b = ground[2] + (horizon[2] - ground[2]) * s;
                }

                int idx = (y * SIZE + x) * 3;
                pixels[idx+0] = (unsigned char)(r * 255.0f);
                pixels[idx+1] = (unsigned char)(g * 255.0f);
                pixels[idx+2] = (unsigned char)(b * 255.0f);
            }
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                     0, GL_RGB8, SIZE, SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE,
                     pixels.data());
    }

    setCommonCubemapParams();
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return c;
}

Cubemap Cubemap::fromFiles(const std::array<std::string,6>& faces)
{
    Cubemap c;
    glGenTextures(1, &c.m_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, c.m_id);

    stbi_set_flip_vertically_on_load(false);
    for (int i = 0; i < 6; ++i)
    {
        int w, h, ch;
        unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &ch, 3);
        if (!data) {
            std::fprintf(stderr, "[Cubemap] failed to load: %s\n", faces[i].c_str());
            continue;
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                     0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    setCommonCubemapParams();
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return c;
}

Cubemap::~Cubemap() { glDeleteTextures(1, &m_id); }

Cubemap::Cubemap(Cubemap&& o) noexcept : m_id(o.m_id) { o.m_id = 0; }

Cubemap& Cubemap::operator=(Cubemap&& o) noexcept
{
    if (this != &o) {
        glDeleteTextures(1, &m_id);
        m_id = o.m_id;
        o.m_id = 0;
    }
    return *this;
}

void Cubemap::bind(unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
}
