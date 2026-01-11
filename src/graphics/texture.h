#pragma once

#include <GL/glew.h>

namespace Graphics {

/**
 * @brief 2D texture wrapper class
 */
class Texture2D
{
public:
    Texture2D(int w, int h, GLenum internalFormat = GL_RGBA32F, GLenum format = GL_RGBA);
    ~Texture2D();

    // Disable copy, allow move
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&&) = default;
    Texture2D& operator=(Texture2D&&) = default;

    void bind(int unit = 0);
    
    GLuint getID() const { return m_id; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    GLuint m_id;
    int m_width;
    int m_height;
};

} // namespace Graphics
