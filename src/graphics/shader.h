#pragma once

#include <GL/glew.h>
#include <string>

namespace Graphics {

/**
 * @brief Shader program wrapper
 */
class Shader
{
public:
    /**
     * @brief Create shader from vertex and fragment source code
     */
    Shader(const char* vertSrc, const char* fragSrc);
    ~Shader();

    // Disable copy, allow move
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&) = default;
    Shader& operator=(Shader&&) = default;

    void use();
    void setInt(const char* name, int value);
    void setFloat(const char* name, float value);
    
    GLuint getID() const { return m_id; }

private:
    GLuint compileShader(GLenum type, const char* src);
    
    GLuint m_id;
};

} // namespace Graphics
