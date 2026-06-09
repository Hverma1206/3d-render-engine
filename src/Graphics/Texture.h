#pragma once

#include <string>

// RAII 2D texture. Loads an image via stb_image, uploads it to an OpenGL
// texture object, builds a mipmap chain, and sets sane wrap/filter state.
// Move-only: owns exactly one GL texture handle.
//
// srgb=true selects an sRGB internal format so the GPU linearizes texels on
// sample. Use it for COLOR maps (albedo/diffuse); use false for DATA maps
// (normal, roughness, metallic, AO) whose values are not gamma-encoded.
class Texture
{
public:
    explicit Texture(const std::string& path, bool srgb = true, bool flipVertically = true);
    ~Texture();

    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // Factory: 1×1 solid-colour textures — used as fallbacks when a mesh has
    // no material map (white diffuse, grey specular, etc.).
    static Texture createWhite();
    static Texture createGrey();  // 0.5 grey — neutral specular fallback

    // Bind to a texture image unit (matches the sampler's set value in GLSL).
    void bind(unsigned int unit = 0) const;

    unsigned int id() const { return m_id; }
    int  width()  const { return m_width; }
    int  height() const { return m_height; }
    bool valid()  const { return m_id != 0; }

private:
    Texture() = default; // used only by createWhite/createGrey factories

    unsigned int m_id = 0;
    int m_width = 0, m_height = 0, m_channels = 0;
};
