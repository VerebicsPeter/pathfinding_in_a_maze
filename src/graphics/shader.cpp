#include "shader.h"
#include <iostream>
#include <vector>

namespace Graphics {

GLuint Shader::compileShader(GLenum type, const char* src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    // Check compilation
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength);
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        std::cerr << "Shader compilation error:\n" << log.data() << std::endl;
    }

    return shader;
}

Shader::Shader(const char* vertSrc, const char* fragSrc)
{
    GLuint vert = compileShader(GL_VERTEX_SHADER, vertSrc);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);

    m_id = glCreateProgram();
    glAttachShader(m_id, vert);
    glAttachShader(m_id, frag);
    glLinkProgram(m_id);

    // Check linking
    GLint success;
    glGetProgramiv(m_id, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLint logLength;
        glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength);
        glGetProgramInfoLog(m_id, logLength, nullptr, log.data());
        std::cerr << "Shader linking error:\n" << log.data() << std::endl;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
}

void Shader::use()
{
    glUseProgram(m_id);
}

void Shader::setInt(const char* name, int value)
{
    glUniform1i(glGetUniformLocation(m_id, name), value);
}

void Shader::setFloat(const char* name, float value)
{
    glUniform1f(glGetUniformLocation(m_id, name), value);
}

Shader::~Shader()
{
    glDeleteProgram(m_id);
}

} // namespace Graphics
