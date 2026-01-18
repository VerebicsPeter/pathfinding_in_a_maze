// TODO:

// - Native maze generation
// - Implement dijkstra and A* using the new cost buffer and 
// relaxation with bucket based delta stepping:
// - Visualize dijkstra and A* costs somehow

// in pseudocode (roughly)
// found = false
// for bucket in sorted(buckets, "asc"):
// # the k-th bucket has indices with dist in [k*delta, (k+1)*delta-1]
// # NOTE: buckets are holding the indices of the current wavefront
//   while bucket not empty and not found:
//     # part of the wavefront that is  
//     found = call("step_bucketed_wavefront", bucket)

// this call means expand ALL nodes IN the bucket,
//  - it may add new nodes to the same bucket that's why we need while
//  - it is correct because if it adds the same node to 2 diff buckets
//  the first one to relax that node WILL be the 'closer' bucket
//  - it terminates either when target is stopped or when every node is relaxed

#include "application.h"
#include "camera.h"
#include "../graphics/shader.h"
#include "../graphics/buffer.h"
#include "../graphics/quad.h"
#include "../compute/cl_context.h"
#include "../compute/cl_program.h"
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
    , m_backtracking(false)
    , m_currentStep(0)
    , m_mazeSize(65)
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

    m_window = SDL_CreateWindow("Pathfinding in a Maze",
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
    m_hostMazeCosts = Maze::createMaze(m_mazeSize, m_algorithm, false);

    if (m_hostMazeCosts.empty())
    {
        throw std::runtime_error("Failed to generate maze");
    }

    // Calculate indices
    auto toIndex = [this](int r, int c) { return r * m_mazeSize + c; };
    m_startIdx = toIndex(1, 1);
    m_targetIdx = toIndex(m_mazeSize - 2, m_mazeSize - 2);

    // Create wall buffer for graphics
    m_costBuffer = std::make_unique<Graphics::SSBO>(
        m_hostMazeCosts.size() * sizeof(int32_t),
        m_hostMazeCosts.data()
    );

    // Initialize distance buffer
    std::vector<int32_t> distances(m_mazeSize * m_mazeSize, -1);
    distances[m_startIdx] = 0;
    
    m_distBuffer = std::make_unique<Graphics::SSBO>(
        distances.size() * sizeof(int32_t),
        distances.data()
    );

    std::vector<int32_t> visited(m_mazeSize * m_mazeSize, 0);
    m_visitBuffer = std::make_unique<Graphics::SSBO>(
        visited.size() * sizeof(int32_t),
        visited.data()
    );

    // Initialize OpenCL buffers for pathfinding
    const int maxWfSize = 2 * (std::max(m_mazeSize, m_mazeSize) - 1);
    const int indexArrSize = 4 * maxWfSize;

    // Create host buffers
    std::vector<int32_t> prevHost(indexArrSize, -1);
    std::vector<int32_t> nextHost(indexArrSize, -1);
    std::vector<uint8_t> foundFlagHost(1, 0);

    if (!Maze::initializeMazeState(
            *m_clContext,
            *m_clProgram,
            m_hostMazeCosts,
            m_mazeSize,
            m_startIdx,
            m_mazeState
        ))
    {
        throw std::runtime_error("Failed to initialize maze.");
    }

    m_currentStep = 0;
    m_currentWavefrontSize = 1;
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
    static int currBacktrackingIdx = -1;
    static int currBacktrackingDst = -1;

    // step backtracking
    if (m_backtracking) {
        int neighbors[] = {
            currBacktrackingIdx - 1,
            currBacktrackingIdx + 1,
            currBacktrackingIdx - m_mazeSize,
            currBacktrackingIdx + m_mazeSize
        };

        int nextIdx = -1;

        for (int i = 0; i < 4; ++i) {
            int n = neighbors[i];
            
            // bounds check
            if (n < 0 || n >= m_mazeSize * m_mazeSize)
                continue;
            
            // skip walls and already backtracked
            if (m_hostMazeCosts[n] < 0 ||
                m_mazeState.visitedFlag[n])
                continue;

            if (m_mazeState.distHost[n] >= 0 &&
                m_mazeState.distHost[n] <= currBacktrackingDst) {
                currBacktrackingDst = m_mazeState.distHost[n];
                nextIdx = n;
            }
        }

        if (nextIdx == -1) {
            // stuck, should not happen if path exists
            m_backtracking = false;
            std::cout << "Backtracking stuck!" << std::endl;
        } else {
            currBacktrackingIdx = nextIdx;
            m_mazeState.visitedFlag[currBacktrackingIdx] = 2; // mark path 
            m_visitBuffer->update(sizeof(int32_t) * m_mazeState.visitedFlag.size(), m_mazeState.visitedFlag.data());
            // done if on start
            m_backtracking = currBacktrackingIdx != m_startIdx;
            std::cout << "Backtracking @" << currBacktrackingIdx << "\n";
        }
    }

    if (m_pathFound)
        return;

    // Run pathfinding step
    m_pathFound = Maze::stepPathfinding(
        m_currentStep,
        m_mazeSize,
        m_currentWavefrontSize,
        m_clContext->getQueue(),
        m_mazeState.kernel,
        m_targetIdx,
        m_mazeState.costBuf,
        m_mazeState.prevBuf,
        m_mazeState.nextBuf,
        m_mazeState.distBuf,
        m_mazeState.foundFlagBuf,
        m_mazeState.prevHost,
        m_mazeState.nextHost,
        m_mazeState.distHost,
        m_mazeState.foundFlagHost
    );

    // Update distance buffer for rendering
    m_clContext->getQueue().enqueueReadBuffer(
        m_mazeState.distBuf,
        CL_TRUE,
        0,
        sizeof(int32_t) * m_mazeState.distHost.size(),
        m_mazeState.distHost.data()
    );
    
    m_distBuffer->update(
        sizeof(int32_t) * m_mazeState.distHost.size(),
        m_mazeState.distHost.data());
    
    if (m_pathFound) {
        m_backtracking = true;
        std::cout << "Backtracking starting...\n";
        currBacktrackingIdx = m_targetIdx;
        currBacktrackingDst = m_mazeState.distHost[m_targetIdx];
    }
}

void Application::renderImgui()
{
    auto onRestart = [&]() {
        // Reinitialize state
        if (!Maze::initializeMazeState(
                *m_clContext,
                *m_clProgram,
                m_hostMazeCosts,
                m_mazeSize,
                m_startIdx,
                m_mazeState
            ))
        {
            throw std::runtime_error("Failed to initialize maze.");
        }

        // Reset wavefront / pathfinding trackers
        const int maxWfSize = 2 * (std::max(m_mazeSize, m_mazeSize) - 1);
        const int indexArrSize = 4 * maxWfSize;

        m_mazeState.prevHost.assign(indexArrSize, -1);
        m_mazeState.nextHost.assign(indexArrSize, -1);
        m_mazeState.distHost.assign(m_mazeSize * m_mazeSize, -1);
        m_mazeState.distHost[m_startIdx] = 0;
        m_mazeState.prevHost[0] = m_startIdx;
        m_mazeState.foundFlagHost.assign(1, 0);

        // Update OpenCL buffers to match
        m_clContext->getQueue().enqueueWriteBuffer(m_mazeState.distBuf, CL_TRUE, 0,
            sizeof(int32_t) * m_mazeState.distHost.size(), m_mazeState.distHost.data());
        m_clContext->getQueue().enqueueWriteBuffer(m_mazeState.prevBuf, CL_TRUE, 0,
            sizeof(int32_t) * m_mazeState.prevHost.size(), m_mazeState.prevHost.data());
        m_clContext->getQueue().enqueueWriteBuffer(m_mazeState.nextBuf, CL_TRUE, 0,
            sizeof(int32_t) * m_mazeState.nextHost.size(), m_mazeState.nextHost.data());
        m_clContext->getQueue().enqueueWriteBuffer(m_mazeState.foundFlagBuf, CL_TRUE, 0,
            sizeof(uint8_t), m_mazeState.foundFlagHost.data());

        // Reset app-level counters
        m_currentStep = 0;
        m_currentWavefrontSize = 1;
        m_pathFound = false;
        m_backtracking = false;

        // reset backtracking visualization
        m_mazeState.visitedFlag.assign(m_mazeSize * m_mazeSize, 0);
        m_visitBuffer->update(sizeof(int32_t) * m_mazeState.visitedFlag.size(), m_mazeState.visitedFlag.data());
    };

    // Selected algorithm
    const char* algorithms[] = { "depthfs", "kruskal" };
    int selectedAlgoIdx = (m_algorithm == "depthfs") ? 0 : 1;

    // Render ImGui
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Stats");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Begin("Debug");
        if (ImGui::Combo("Maze Algo", &selectedAlgoIdx, algorithms, IM_ARRAYSIZE(algorithms))) {
            m_algorithm = algorithms[selectedAlgoIdx];
        }

        ImGui::Text("Gen algorithm: %s", m_algorithm.c_str());

        ImGui::InputInt("Maze Size", &m_mazeSize, 1, 25);

        if (ImGui::Button("Restart Map")) {
            initMaze();
            initGraphics();
            onRestart();
        }
        
        if (ImGui::Button("Restart Run")) {
            onRestart();
        }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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
    m_costBuffer->bind(0);
    m_distBuffer->bind(1);
    m_visitBuffer->bind(2);

    // Draw
    m_quad->draw();

    renderImgui();  

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
