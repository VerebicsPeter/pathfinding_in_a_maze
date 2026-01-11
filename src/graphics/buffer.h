#pragma once

#include <GL/glew.h>
#include <cstddef>

namespace Graphics {

/**
 * @brief Shader Storage Buffer Object wrapper
 */
class SSBO
{
public:
    SSBO(size_t size, void* data);
    ~SSBO();

    // Disable copy, allow move
    SSBO(const SSBO&) = delete;
    SSBO& operator=(const SSBO&) = delete;
    SSBO(SSBO&&) = default;
    SSBO& operator=(SSBO&&) = default;

    void update(size_t size, void* data);
    void bind(int binding);
    
    GLuint getID() const { return m_id; }

private:
    GLuint m_id;
};

} // namespace Graphics
