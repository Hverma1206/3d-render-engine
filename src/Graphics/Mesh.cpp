#include "Graphics/Mesh.h"

#include <glad/glad.h>

#include <cstddef>   // offsetof
#include <utility>

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices)
    : m_vertices(std::move(vertices))
    , m_indices(std::move(indices))
    , m_vertexCount(m_vertices.size())
    , m_indexCount(m_indices.size())
{
    upload();
}

Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
}

Mesh::Mesh(Mesh&& other) noexcept
    : m_vertices(std::move(other.m_vertices))
    , m_indices(std::move(other.m_indices))
    , m_vao(other.m_vao), m_vbo(other.m_vbo), m_ebo(other.m_ebo)
    , m_vertexCount(other.m_vertexCount), m_indexCount(other.m_indexCount)
{
    other.m_vao = other.m_vbo = other.m_ebo = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other)
    {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);

        m_vertices    = std::move(other.m_vertices);
        m_indices     = std::move(other.m_indices);
        m_vao         = other.m_vao;
        m_vbo         = other.m_vbo;
        m_ebo         = other.m_ebo;
        m_vertexCount = other.m_vertexCount;
        m_indexCount  = other.m_indexCount;

        other.m_vao = other.m_vbo = other.m_ebo = 0;
    }
    return *this;
}

void Mesh::upload()
{
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)),
                 m_vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_indices.size() * sizeof(unsigned int)),
                 m_indices.data(), GL_STATIC_DRAW);

    // Attribute layout is defined by Vertex.h and fixed for all shaders.
    // Using offsetof() is safe on a standard-layout struct with no padding tricks.
    const GLsizei stride = sizeof(Vertex);

    // location 0: position (vec3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);

    // location 1: normal (vec3)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);

    // location 2: texCoords (vec2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(offsetof(Vertex, texCoords)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Mesh::draw() const
{
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(m_indexCount),
                   GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}
