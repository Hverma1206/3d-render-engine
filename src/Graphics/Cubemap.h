#pragma once
#include <glad/glad.h>
#include <string>
#include <array>

// Phase 16 — Cubemap texture (GL_TEXTURE_CUBE_MAP).
// createSky() builds a procedural 64x64 sky cubemap entirely on the CPU —
// no external assets needed, yet demonstrates the full cubemap upload API.
class Cubemap
{
public:
    static Cubemap createSky();
    static Cubemap fromFiles(const std::array<std::string,6>& faces); // +X −X +Y −Y +Z −Z

    ~Cubemap();
    Cubemap(Cubemap&&) noexcept;
    Cubemap& operator=(Cubemap&&) noexcept;
    Cubemap(const Cubemap&)            = delete;
    Cubemap& operator=(const Cubemap&) = delete;

    void   bind(unsigned int unit = 0) const;
    GLuint id() const { return m_id; }

private:
    Cubemap() = default;
    GLuint m_id = 0;
};
