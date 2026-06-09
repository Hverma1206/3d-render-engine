#include "Graphics/Shader.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <utility>

namespace
{
    // Read an entire text file into a string. Returns false on failure.
    bool readFile(const std::string& path, std::string& out)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            std::fprintf(stderr, "[Shader] failed to open '%s'\n", path.c_str());
            return false;
        }
        std::stringstream ss;
        ss << file.rdbuf();
        out = ss.str();
        return true;
    }

    // Compile one shader stage; logs a labelled error and returns 0 on failure.
    unsigned int compile(GLenum type, const std::string& src, const std::string& path)
    {
        unsigned int shader = glCreateShader(type);
        const char* c = src.c_str();
        glShaderSource(shader, 1, &c, nullptr);
        glCompileShader(shader);

        int ok = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            char log[1024];
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            std::fprintf(stderr, "[Shader] compile error in '%s':\n%s\n",
                         path.c_str(), log);
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }
} 

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
{
    std::string vsrc, fsrc;
    if (!readFile(vertexPath, vsrc) || !readFile(fragmentPath, fsrc))
        return; 

    unsigned int vs = compile(GL_VERTEX_SHADER,   vsrc, vertexPath);
    unsigned int fs = compile(GL_FRAGMENT_SHADER, fsrc, fragmentPath);
    if (vs == 0 || fs == 0)
    {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return;
    }

    m_id = glCreateProgram();
    glAttachShader(m_id, vs);
    glAttachShader(m_id, fs);
    glLinkProgram(m_id);

    int ok = 0;
    glGetProgramiv(m_id, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[1024];
        glGetProgramInfoLog(m_id, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[Shader] link error (%s + %s):\n%s\n",
                     vertexPath.c_str(), fragmentPath.c_str(), log);
        glDeleteProgram(m_id);
        m_id = 0;
    }

    // Shader objects are baked into the program at link time; free them now.
    glDeleteShader(vs);
    glDeleteShader(fs);
}

Shader::~Shader()
{
    glDeleteProgram(m_id); // glDeleteProgram(0) is a safe no-op
}
Shader::Shader(Shader&& other) noexcept
    : m_id(other.m_id), m_uniformCache(std::move(other.m_uniformCache))
{
    other.m_id = 0;
}
Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other)
    {
        glDeleteProgram(m_id);         
        m_id           = other.m_id;
        m_uniformCache = std::move(other.m_uniformCache);
        other.m_id     = 0;
    }
    return *this;
}
void Shader::use() const
{
    glUseProgram(m_id);
}

int Shader::uniformLocation(const std::string& name)
{
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end())
        return it->second;

    int loc = glGetUniformLocation(m_id, name.c_str());
    if (loc == -1)
        std::fprintf(stderr, "[Shader] uniform '%s' not found (or optimized out)\n",
                     name.c_str());
    m_uniformCache.emplace(name, loc); 
    return loc;
}

void Shader::setBool (const std::string& n, bool  v) { glProgramUniform1i(m_id, uniformLocation(n), v ? 1 : 0); }
void Shader::setInt  (const std::string& n, int   v) { glProgramUniform1i(m_id, uniformLocation(n), v); }
void Shader::setFloat(const std::string& n, float v) { glProgramUniform1f(m_id, uniformLocation(n), v); }
void Shader::setVec2 (const std::string& n, const glm::vec2& v) { glProgramUniform2fv(m_id, uniformLocation(n), 1, glm::value_ptr(v)); }
void Shader::setVec3 (const std::string& n, const glm::vec3& v) { glProgramUniform3fv(m_id, uniformLocation(n), 1, glm::value_ptr(v)); }
void Shader::setVec4 (const std::string& n, const glm::vec4& v) { glProgramUniform4fv(m_id, uniformLocation(n), 1, glm::value_ptr(v)); }
void Shader::setMat3 (const std::string& n, const glm::mat3& m) { glProgramUniformMatrix3fv(m_id, uniformLocation(n), 1, GL_FALSE, glm::value_ptr(m)); }
void Shader::setMat4 (const std::string& n, const glm::mat4& m) { glProgramUniformMatrix4fv(m_id, uniformLocation(n), 1, GL_FALSE, glm::value_ptr(m)); }
