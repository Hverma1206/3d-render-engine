#pragma once

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>

// RAII wrapper around a linked OpenGL shader program.
//
// Loads + compiles a vertex and fragment shader from files, links them into a
// program, and caches uniform locations so repeated set* calls don't re-query
// the driver. Move-only: it owns exactly one GL program handle.
//
// Uniform setters use glProgramUniform* (core in OpenGL 4.1), so they do NOT
// require the program to be bound first -- one less stateful footgun than the
// legacy glUseProgram + glUniform pattern.
class Shader
{
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    void use() const;                      // glUseProgram(m_id)
    unsigned int id() const { return m_id; }
    bool valid() const { return m_id != 0; }

    void setBool (const std::string& name, bool  value);
    void setInt  (const std::string& name, int   value);
    void setFloat(const std::string& name, float value);
    void setVec2 (const std::string& name, const glm::vec2& v);
    void setVec3 (const std::string& name, const glm::vec3& v);
    void setVec4 (const std::string& name, const glm::vec4& v);
    void setMat3 (const std::string& name, const glm::mat3& m);
    void setMat4 (const std::string& name, const glm::mat4& m);

private:
    int uniformLocation(const std::string& name);

    unsigned int m_id = 0;
    std::unordered_map<std::string, int> m_uniformCache;
};
