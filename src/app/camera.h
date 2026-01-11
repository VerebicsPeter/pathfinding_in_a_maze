#pragma once

#include <glm/glm.hpp>

namespace App {

/**
 * @brief 2D camera for viewing the maze
 */
class Camera2D
{
public:
    Camera2D();
    ~Camera2D() = default;

    void setPosition(const glm::vec2& pos) { m_position = pos; }
    void move(const glm::vec2& delta) { m_position += delta; }
    void setZoom(float zoom) { m_zoom = zoom; }
    void adjustZoom(float factor) { m_zoom *= factor; }

    glm::vec2 getPosition() const { return m_position; }
    float getZoom() const { return m_zoom; }

    /**
     * @brief Get view-projection matrix
     * @param screenWidth Screen width in pixels
     * @param screenHeight Screen height in pixels
     * @return Combined view-projection matrix
     */
    glm::mat4 getViewProjectionMatrix(float screenWidth, float screenHeight) const;

private:
    glm::vec2 m_position;
    float m_zoom;
};

} // namespace App
