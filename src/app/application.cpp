#include "application.h"
#include "camera.h"
#include "../graphics/shader.h"
#include "../graphics/buffer.h"
#include "../graphics/quad.h"
#include "../compute/cl_context.h"
#include "../compute/cl_program.h"
#include "../maze/maze.h"
#include "../maze/pathfinding.h"
#include "../utils/file_utils.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define CL_HPP_TARGET_OPENCL_VERSION 300
#include <CL/opencl.hpp>

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

#include <iostream>
#include <algorithm>

namespace App {

Application::Application(int width, int height)
    : m_width(width)
    , m_height(height)
    , m_window(nullptr)
    , m_glContext(nullptr)
    , m_running(true)
    , m_pathFound(false)
    , m_currentStep(0)
    , m_mazeSize(255)
    , m_currentWavefrontSize(1)
{
    initSDL();
    initOpenGL();
    initOpenCL();
    initMaze();
    initGraphics();
    initImGui();
}

Application::~Application()
{
    cleanup();
}

void Application::initSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        throw std::runtime_error(std::string("SDL init failed: ") + SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    m_window = SDL_CreateWindow("Pathfinding Visualization",
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                m_width, m_height,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!m_window)
    {
        throw std::runtime_error("Failed to create SDL window");
    }

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext)
    {
        throw std::runtime_error("Failed to create SDL GL context");
    }
}

void Application::initOpenGL()
{
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        throw std::runtime_error("GLEW init failed");
    }

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
}

void Application::initOpenCL()
{
    m_clContext = std::make_unique<Compute::CLContext>(m_window, 0, 0);
    m_clProgram = std::make_unique<Compute::CLProgram>(
        "assets/kernels/step_wavefront.cl",
        m_clContext->getContext(),
        m_clContext->getDevice()
    );
}

void Application::initMaze()
{
    // Generate maze
    std::string algorithm = "kruskal";
    m_hostMazeWalls = Maze::createMaze(m_mazeSize, algorithm);

    if (m_hostMazeWalls.empty())
    {
        throw std::runtime_error("Failed to generate maze");
    }

    // Calculate indices
    auto toIndex = [this](int r, int c) { return r * m_mazeSize + c; };
    m_startIdx = toIndex(1, 1);
    m_targetIdx = toIndex(m_mazeSize - 2, m_mazeSize - 2);

    // Create wall buffer for graphics
    m_wallBuffer = std::make_unique<Graphics::SSBO>(
        m_hostMazeWalls.size() * sizeof(int32_t),
        m_hostMazeWalls.data()
    );

    // Initialize distance buffer
    std::vector<int32_t> distances(m_mazeSize * m_mazeSize, -1);
    distances[m_startIdx] = 0;
    
    m_distBuffer = std::make_unique<Graphics::SSBO>(
        distances.size() * sizeof(int32_t),
        distances.data()
    );

    // Initialize OpenCL buffers for pathfinding
    const int maxWfSize = 2 * (std::max(m_mazeSize, m_mazeSize) - 1);
    const int indexArrSize = 4 * maxWfSize;

    // Create host buffers
    std::vector<int32_t> prevHost(indexArrSize, -1);
    std::vector<int32_t> nextHost(indexArrSize, -1);
    std::vector<uint8_t> foundFlagHost(1, 0);

    prevHost[0] = m_startIdx;

    // Create OpenCL buffers (stored as class members would be better, but we'll use local for now)
    // For simplicity, we'll handle this in the update() method
}

void Application::initGraphics()
{
    // Load shaders
    std::string vertSrc = Utils::readFile("assets/shaders/shader.vert");
    std::string fragSrc = Utils::readFile("assets/shaders/shader.frag");

    if (vertSrc.empty() || fragSrc.empty())
    {
        throw std::runtime_error("Failed to load shader files");
    }

    m_shader = std::make_unique<Graphics::Shader>(vertSrc.c_str(), fragSrc.c_str());
    m_quad = std::make_unique<Graphics::Quad>();

    // Setup shader uniforms
    m_shader->use();
    m_shader->setInt("imageWidth", m_mazeSize);
    m_shader->setInt("imageHeight", m_mazeSize);

    // Setup camera
    m_camera = std::make_unique<Camera2D>();
    float camX = -static_cast<float>(m_width) / 2.0f;
    float camY = -static_cast<float>(m_height) / 2.0f;
    m_camera->setPosition(glm::vec2(camX, camY));
    m_camera->setZoom(8.0f);

    std::cout << "Camera position: " << camX << " " << camY << std::endl;
}

void Application::initImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(m_window, m_glContext);
    ImGui_ImplOpenGL3_Init("#version 450");

    std::cout << "ImGui initialized" << std::endl;
}

void Application::handleEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);

        if (event.type == SDL_QUIT)
        {
            m_running = false;
        }
        else if (event.type == SDL_KEYDOWN)
        {
            const float moveSpeed = 10.0f;
            const float zoomFactor = 1.1f;

            switch (event.key.keysym.sym)
            {
            case SDLK_w:
                m_camera->move(glm::vec2(0.0f, moveSpeed));
                break;
            case SDLK_s:
                m_camera->move(glm::vec2(0.0f, -moveSpeed));
                break;
            case SDLK_a:
                m_camera->move(glm::vec2(-moveSpeed, 0.0f));
                break;
            case SDLK_d:
                m_camera->move(glm::vec2(moveSpeed, 0.0f));
                break;
            case SDLK_q:
                m_camera->adjustZoom(zoomFactor);
                break;
            case SDLK_e:
                m_camera->adjustZoom(1.0f / zoomFactor);
                break;
            case SDLK_ESCAPE:
                m_running = false;
                break;
            }
        }
    }
}

void Application::update()
{
    if (m_pathFound)
        return;

    // We need to maintain OpenCL buffers across frames
    // For simplicity, we'll recreate them each time (inefficient but works)
    // A better approach would be to store them as class members
    
    static bool initialized = false;
    static cl::Buffer wallBuf;
    static cl::Buffer prevBuf;
    static cl::Buffer nextBuf;
    static cl::Buffer distBuf;
    static cl::Buffer foundFlagBuf;
    static std::vector<int32_t> prevHost;
    static std::vector<int32_t> nextHost;
    static std::vector<int32_t> distHost;
    static std::vector<uint8_t> foundFlagHost;
    static cl::Kernel kernel;

    if (!initialized)
    {
        // Initialize buffers
        // Wall buffer is already generated in initMaze
        
        const int maxWfSize = 2 * (std::max(m_mazeSize, m_mazeSize) - 1);
        const int indexArrSize = 4 * maxWfSize;

        prevHost.resize(indexArrSize, -1);
        nextHost.resize(indexArrSize, -1);
        distHost.resize(m_mazeSize * m_mazeSize, -1);
        foundFlagHost.resize(1, 0);

        distHost[m_startIdx] = 0;
        prevHost[0] = m_startIdx;

        // Create OpenCL buffers
        wallBuf = cl::Buffer(
            m_clContext->getContext(),
            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            sizeof(int32_t) * m_hostMazeWalls.size(),
            m_hostMazeWalls.data()
        );

        prevBuf = cl::Buffer(
            m_clContext->getContext(),
            CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
            sizeof(int32_t) * prevHost.size(),
            prevHost.data()
        );

        nextBuf = cl::Buffer(
            m_clContext->getContext(),
            CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
            sizeof(int32_t) * nextHost.size(),
            nextHost.data()
        );

        distBuf = cl::Buffer(
            m_clContext->getContext(),
            CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
            sizeof(int32_t) * distHost.size(),
            distHost.data()
        );

        foundFlagBuf = cl::Buffer(
            m_clContext->getContext(),
            CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
            sizeof(uint8_t) * foundFlagHost.size(),
            foundFlagHost.data()
        );

        // Create kernel
        kernel = cl::Kernel(m_clProgram->getProgram(), "expand_wave_idxs");

        initialized = true;
    }

    // Run pathfinding step
    m_pathFound = Maze::stepPathfinding(
        m_currentStep,
        m_mazeSize,
        m_currentWavefrontSize,
        m_clContext->getQueue(),
        kernel,
        m_targetIdx,
        wallBuf,
        prevBuf,
        nextBuf,
        distBuf,
        foundFlagBuf,
        prevHost,
        nextHost,
        distHost,
        foundFlagHost
    );

    // Update distance buffer for rendering
    m_clContext->getQueue().enqueueReadBuffer(
        distBuf,
        CL_TRUE,
        0,
        sizeof(int32_t) * distHost.size(),
        distHost.data()
    );
    
    m_distBuffer->update(sizeof(int32_t) * distHost.size(), distHost.data());
}

void Application::render()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_shader->use();

    // Set view-projection matrix
    glm::mat4 vp = m_camera->getViewProjectionMatrix(
        static_cast<float>(m_width),
        static_cast<float>(m_height)
    );
    GLint vpLoc = glGetUniformLocation(m_shader->getID(), "vp");
    glUniformMatrix4fv(vpLoc, 1, GL_FALSE, &vp[0][0]);

    m_shader->setInt("maxDist", ++m_currentStep);

    // Bind buffers
    m_wallBuffer->bind(0);
    m_distBuffer->bind(1);

    // Draw
    m_quad->draw();

    // Render ImGui
    // Start the Dear ImGui frame
    // We need to start frames for both the platform backend (SDL2) and the renderer backend (OpenGL3)
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(m_window);
}

void Application::run()
{
    while (m_running)
    {
        handleEvents();
        update();
        render();
    }
}

void Application::cleanup()
{
    // Smart pointers will handle cleanup automatically
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (m_glContext)
    {
        SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;
    }

    if (m_window)
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    SDL_Quit();
}

} // namespace App
