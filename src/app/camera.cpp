#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace App {

Camera2D::Camera2D()
    : m_position(0.0f, 0.0f)
    , m_zoom(1.0f)
{
}

glm::mat4 Camera2D::getViewProjectionMatrix(float screenWidth, float screenHeight) const
{
    glm::mat4 proj = glm::ortho(0.0f, screenWidth, 0.0f, screenHeight);
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(-m_position, 0.0f));
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(m_zoom, m_zoom, 1.0f));
    return proj * view * scale;
}

} // namespace App
