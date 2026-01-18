#pragma once

#include <SDL2/SDL.h>
#include <memory>
#include <vector>
#include "../maze/maze.h"

// Forward declarations
namespace Graphics {
    class Shader;
    class SSBO;
    class Quad;
}

namespace Compute {
    class CLContext;
    class CLProgram;
}

namespace App {
    class Camera2D;
}

namespace App {

/**
 * @brief Main application class
 */
class Application
{
public:
    Application(int width, int height);
    ~Application();

    // Disable copy and move
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    /**
     * @brief Run the main application loop
     */
    void run();

private:
    void initSDL();
    void initOpenGL();
    void initOpenCL();
    void initMaze();
    void initGraphics();
    void initImGui();
    
    void handleEvents();
    void update();
    void render();
    void renderImgui();
    void cleanup();

    // Window properties
    int m_width;
    int m_height;
    SDL_Window* m_window;
    SDL_GLContext m_glContext;

    // OpenCL
    std::unique_ptr<Compute::CLContext> m_clContext;
    std::unique_ptr<Compute::CLProgram> m_clProgram;

    // Graphics
    std::unique_ptr<Graphics::Shader> m_shader;
    std::unique_ptr<Graphics::SSBO> m_costBuffer;
    std::unique_ptr<Graphics::SSBO> m_distBuffer;
    std::unique_ptr<Graphics::SSBO> m_visitBuffer; // ssbo for backtracking visited state
    std::unique_ptr<Graphics::Quad> m_quad;
    std::unique_ptr<Camera2D> m_camera;

    // Application state
    bool m_running;
    bool m_pathFound;
    bool m_backtracking;
    int m_currentStep;

    std::string m_algorithm = "kruskal"; // maze gen algo

    // Maze data
    int m_mazeSize;
    int m_startIdx;
    int m_targetIdx;
    Maze::MazeState m_mazeState;
    
    // Pathfinding state
    int m_currentWavefrontSize;

    // Maze memory
    std::vector<int32_t> m_hostMazeCosts;
};

} // namespace App
