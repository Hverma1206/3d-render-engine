#pragma once

#include "Graphics/Mesh.h"
#include "Graphics/Texture.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Shader;

// Loads a model file via Assimp and stores it as a flat list of Mesh+Texture
// pairs. Walks the scene node tree and flattens it — hierarchy is not needed
// for rendering in this project. Move-only (owns Mesh GPU handles).
//
// draw(shader) binds diffuse->unit0, specular->unit1 and calls mesh.draw().
// drawGeometry() skips material binding — used for depth/shadow passes.
class Model
{
public:
    explicit Model(const std::string& path);

    void draw(Shader& shader) const;
    void drawGeometry() const;

    bool   valid()     const { return !m_entries.empty(); }
    size_t meshCount() const { return m_entries.size();   }

private:
    struct Entry {
        Mesh                     mesh;
        std::shared_ptr<Texture> diffuse;   // unit 0 — nullptr -> white fallback
        std::shared_ptr<Texture> specular;  // unit 1 — nullptr -> grey fallback
    };

    // Forward-declare Assimp types to keep the header clean.
    void  processNode(const struct aiNode* node, const struct aiScene* scene);
    Entry processMesh(const struct aiMesh* mesh, const struct aiScene* scene);
    std::shared_ptr<Texture> loadTex(const struct aiMaterial* mat,
                                     int aiType, bool srgb);

    std::vector<Entry>                                   m_entries;
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_texCache;
    std::string                                          m_directory;

    // Shared fallback textures created once at construction.
    std::shared_ptr<Texture> m_fallbackDiffuse;
    std::shared_ptr<Texture> m_fallbackSpecular;
};
