#include "Graphics/Model.h"
#include "Graphics/Shader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <cstdio>

Model::Model(const std::string& path)
{
    // Shared 1×1 fallback textures — created once, shared by every entry that
    // lacks a material map.
    m_fallbackDiffuse  = std::make_shared<Texture>(Texture::createWhite());
    m_fallbackSpecular = std::make_shared<Texture>(Texture::createGrey());

    // Store the directory so relative texture paths can be resolved.
    m_directory = path.substr(0, path.find_last_of("/\\"));

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate       |   // all faces -> triangles
        aiProcess_FlipUVs           |   // flip UVs to match OpenGL origin (bottom-left)
        aiProcess_GenSmoothNormals  |   // generate normals if missing
        aiProcess_CalcTangentSpace);    // needed for normal maps (later phases)

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
    {
        std::fprintf(stderr, "[Model] Assimp error: %s\n", importer.GetErrorString());
        return;
    }

    processNode(scene->mRootNode, scene);
    std::printf("[Model] loaded '%s' — %zu mesh(es)\n", path.c_str(), m_entries.size());
}

void Model::processNode(const aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
        m_entries.push_back(processMesh(scene->mMeshes[node->mMeshes[i]], scene));

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
        processNode(node->mChildren[i], scene);
}

Model::Entry Model::processMesh(const aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(mesh->mNumVertices);

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex v{};
        v.position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};

        if (mesh->HasNormals())
            v.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};

        // Assimp supports up to 8 UV channels; we only use channel 0.
        if (mesh->mTextureCoords[0])
            v.texCoords = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};

        vertices.push_back(v);
    }

    for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
        for (unsigned int j = 0; j < mesh->mFaces[f].mNumIndices; ++j)
            indices.push_back(mesh->mFaces[f].mIndices[j]);

    std::shared_ptr<Texture> diff, spec;
    if (mesh->mMaterialIndex < scene->mNumMaterials)
    {
        const aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
        diff = loadTex(mat, aiTextureType_DIFFUSE,  /*srgb=*/true);
        spec = loadTex(mat, aiTextureType_SPECULAR, /*srgb=*/false);
    }
    // Construct Entry with all members at once (Mesh has no default ctor).
    return Entry{
        Mesh(std::move(vertices), std::move(indices)),
        diff ? diff : m_fallbackDiffuse,
        spec ? spec : m_fallbackSpecular
    };
}

std::shared_ptr<Texture> Model::loadTex(const aiMaterial* mat, int aiTypeInt, bool srgb)
{
    const aiTextureType type = static_cast<aiTextureType>(aiTypeInt);
    if (mat->GetTextureCount(type) == 0) return nullptr;

    aiString rel;
    mat->GetTexture(type, 0, &rel);
    const std::string full = m_directory + "/" + rel.C_Str();

    auto it = m_texCache.find(full);
    if (it != m_texCache.end()) return it->second;

    auto tex = std::make_shared<Texture>(full, srgb);
    m_texCache[full] = tex;
    return tex;
}

void Model::draw(Shader& shader) const
{
    shader.setInt("uMaterial.diffuse",  0);
    shader.setInt("uMaterial.specular", 1);
    for (const auto& e : m_entries)
    {
        e.diffuse->bind(0);
        e.specular->bind(1);
        e.mesh.draw();
    }
}

void Model::drawGeometry() const
{
    for (const auto& e : m_entries)
        e.mesh.draw();
}
