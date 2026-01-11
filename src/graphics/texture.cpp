#include "texture.h"

namespace Graphics {

Texture2D::Texture2D(int w, int h, GLenum internalFormat, GLenum format)
    : m_width(w), m_height(h)
{
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Texture2D::bind(int unit)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

Texture2D::~Texture2D()
{
    glDeleteTextures(1, &m_id);
}

} // namespace Graphics
