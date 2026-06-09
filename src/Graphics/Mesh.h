#pragma once

#include "Graphics/Vertex.h"

#include <vector>

class Shader; // forward-declare: Mesh::draw takes a bound shader by ref (for future UBO/texture helpers)

// RAII renderable mesh. Owns one VAO, one VBO, and one EBO. Move-only.
// The vertex layout (pos/normal/uv at locations 0/1/2) is fixed by Vertex.h
// and assumed by every shader that renders geometry from this class.
class Mesh
{
public:
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);
    ~Mesh();

    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void draw() const; // glDrawElements with this VAO

    unsigned int vertexCount() const { return static_cast<unsigned int>(m_vertexCount); }
    unsigned int indexCount()  const { return static_cast<unsigned int>(m_indexCount); }

private:
    void upload(); // builds VAO/VBO/EBO from m_vertices/m_indices

    std::vector<Vertex>       m_vertices;
    std::vector<unsigned int> m_indices;

    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_ebo = 0;

    size_t m_vertexCount = 0;
    size_t m_indexCount  = 0;
};
