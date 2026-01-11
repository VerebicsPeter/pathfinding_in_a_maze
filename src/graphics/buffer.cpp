#include "buffer.h"

namespace Graphics {

SSBO::SSBO(size_t size, void* data)
{
    glGenBuffers(1, &m_id);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_id);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_DRAW);
}

void SSBO::update(size_t size, void* data)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_id);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_DRAW);
}

void SSBO::bind(int binding)
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_id);
}

SSBO::~SSBO()
{
    glDeleteBuffers(1, &m_id);
}

} // namespace Graphics
