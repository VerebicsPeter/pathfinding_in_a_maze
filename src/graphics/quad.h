#pragma once

#include <GL/glew.h>

namespace Graphics {

/**
 * @brief Full-screen quad for rendering
 */
class Quad
{
public:
    Quad();
    ~Quad();

    // Disable copy, allow move
    Quad(const Quad&) = delete;
    Quad& operator=(const Quad&) = delete;
    Quad(Quad&&) = default;
    Quad& operator=(Quad&&) = default;

    void draw();

private:
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;
};

} // namespace Graphics
