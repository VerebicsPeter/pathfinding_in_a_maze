#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define CL_HPP_TARGET_OPENCL_VERSION 300  // or 120, 200, 210
#include <CL/opencl.hpp>
#include <CL/cl_gl.h>

#ifdef _WIN32
#include <windows.h>
#include <GL/wglew.h>
#elif __linux__
#include <EGL/egl.h>
#include <CL/cl_egl.h>
#include <SDL2/SDL_syswm.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

std::string readFile(const std::string &filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

struct Maze
{
    uint W;
    uint H;
    std::vector<int32_t> Wall;
    std::vector<int32_t> Cost;
    // TODO: Cost buffer
};

std::vector<int32_t> create_maze(const uint size, const std::string& algo)
{
    char cmd[256];
    std::snprintf(cmd, sizeof(cmd), "python gen_maze.py %u \"%s\"", size, algo.c_str());
    int ret = std::system(cmd);
    if (ret != 0)
    {
        std::cerr << "Error while executing python script." << std::endl;
        return {};
    }

    std::vector<uint8_t> mazeBytes(size * size);

    std::ifstream file("maze.bin", std::ios::binary);
    file.read(reinterpret_cast<char *>(mazeBytes.data()), mazeBytes.size());
    file.close();

    std::cout << "Generated maze.\n";

    std::vector<int32_t> mazeData(mazeBytes.size());
    for (size_t i = 0; i < mazeBytes.size(); ++i)
        mazeData[i] = static_cast<int32_t>(mazeBytes[i]);

    return mazeData;
}

class Texture2D
{
    GLuint m_id;
    int m_w, m_h;

public:
    Texture2D(int w, int h, GLenum internalFormat = GL_RGBA32F, GLenum format = GL_RGBA)
        : m_w(w), m_h(h)
    {
        glGenTextures(1, &m_id);
        glBindTexture(GL_TEXTURE_2D, m_id);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    void bind(int unit = 0)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, m_id);
    }

    GLuint getID() const { return m_id; }
    int getW() const { return m_w; }
    int getH() const { return m_h; }

    ~Texture2D()
    {
        glDeleteTextures(1, &m_id);
    }
};

class Shader
{
    GLuint m_id;

    GLuint compileShader(GLenum type, const char *src)
    {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        // Check compilation
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            GLint logLength;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<char> log(logLength);
            glGetShaderInfoLog(shader, logLength, nullptr, log.data());
            std::cerr << "Shader compilation error:\n"
                      << log.data() << std::endl;
        }

        return shader;
    }

public:
    // Constructor for vertex + fragment shaders
    Shader(const char *vertSrc, const char *fragSrc)
    {
        GLuint vert = compileShader(GL_VERTEX_SHADER, vertSrc);
        GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);

        m_id = glCreateProgram();
        glAttachShader(m_id, vert);
        glAttachShader(m_id, frag);
        glLinkProgram(m_id);

        // Check linking
        GLint success;
        glGetProgramiv(m_id, GL_LINK_STATUS, &success);
        if (!success)
        {
            GLint logLength;
            glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<char> log(logLength);
            glGetProgramInfoLog(m_id, logLength, nullptr, log.data());
            std::cerr << "Shader linking error:\n"
                      << log.data() << std::endl;
        }

        glDeleteShader(vert);
        glDeleteShader(frag);
    }
    void use()
    {
        glUseProgram(m_id);
    }
    void setI(const char *name, int value)
    {
        glUniform1i(glGetUniformLocation(m_id, name), value);
    }
    void setF(const char *name, float value)
    {
        glUniform1f(glGetUniformLocation(m_id, name), value);
    }
    GLuint getID() const { return m_id; }
    ~Shader()
    {
        glDeleteProgram(m_id);
    }
};

class SSBO
{
    GLuint m_id;

public:
    SSBO(size_t size, void *data)
    {
        glGenBuffers(1, &m_id);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_id);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_DRAW);
    }

    void update(size_t size, void *data)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_id);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_DRAW);
    }

    void bind(int binding) { glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_id); }
    GLuint getID() const { return m_id; }

    ~SSBO() { glDeleteBuffers(1, &m_id); }
};

class Quad
{
    GLuint m_vao, m_vbo;

public:
    Quad()
    {
        float vertices[] = {
            // positions  // texcoords (flipped v)
            -1.0f, -1.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 1.0f, 1.0f,
             1.0f,  1.0f, 1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f, 0.0f};

        GLuint indices[] = {0, 1, 2, 2, 3, 0};

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        GLuint ebo;
        glGenBuffers(1, &ebo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    void draw()
    {
        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    ~Quad()
    {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
    }
};

struct AppContext
{
    uint Width;
    uint Height;
    SDL_Window *Window;
    SDL_GLContext glContext;
    std::vector<cl_platform_id> clPlatforms;
};

struct OCLContext
{
    cl_platform_id clPlatform;
    cl_device_id clDevice;
    cl_context clContext;
    cl_command_queue clQueue;
};

AppContext CreateAppContext(uint width, uint height)
{
    AppContext appContext = {};
    appContext.Width = width;
    appContext.Height = height;

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "SDL init failed: " << SDL_GetError() << "\n";
        return appContext;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window *window = SDL_CreateWindow("Example", 100, 100, width, height,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window)
    {
        std::cerr << "Failed to create SDL window\n";
        return appContext;
    }
    appContext.Window = window;

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext)
    {
        std::cerr << "Failed to create SDL GL context\n";
        return appContext;
    }
    appContext.glContext = glContext;

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "GLEW init failed\n";
        return appContext;
    }

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << "\n";

    // --- OpenCL initialization --- //
    cl_uint numPlatforms = 0;
    clGetPlatformIDs(0, nullptr, &numPlatforms);
    if (numPlatforms == 0)
    {
        std::cerr << "No OpenCL platforms found\n";
        return appContext;
    }

    appContext.clPlatforms.resize(numPlatforms);
    clGetPlatformIDs(numPlatforms, appContext.clPlatforms.data(), nullptr);

    std::cout << "Number of OpenCL platforms: " << numPlatforms << "\n";

    // Print platform names
    for (cl_uint i = 0; i < numPlatforms; ++i)
    {
        char name[256];
        clGetPlatformInfo(appContext.clPlatforms[i], CL_PLATFORM_NAME, sizeof(name), name, nullptr);
        std::cout << "Platform " << i << ": " << name << "\n";
    }

    return appContext;
}

OCLContext CreateOCLContext(AppContext *appContext, cl_uint platformIndex, cl_uint deviceIndex)
{
    cl_platform_id platform = appContext->clPlatforms[platformIndex];

#ifdef _WIN32
    // WGL interop
    cl_context_properties propsC[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)platform,
        CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
        CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
        0};
#elif __linux__
    // EGL interop
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(appContext->Window, &wmInfo))
    {
        std::cerr << "Failed to get WM info\n";
        return OCLContext{};
    }

    EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(eglDisplay, nullptr, nullptr);
    EGLContext eglContext = eglGetCurrentContext();

    cl_context_properties propsC[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)platform,
        CL_GL_CONTEXT_KHR, (cl_context_properties)eglContext,
        CL_EGL_DISPLAY_KHR, (cl_context_properties)eglDisplay,
        0};
#endif
    cl_uint numDevices = 0;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);

    if (numDevices == 0)
    {
        std::cerr << "No OpenCL devices found\n";
        return OCLContext{};
    }
    if (numDevices <= deviceIndex)
    {
        deviceIndex = 0;
        std::cout << "Using default device\n";
    }

    std::vector<cl_device_id> devices(numDevices);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numDevices, devices.data(), nullptr);
    cl_device_id device = devices[deviceIndex];

    cl_int err;
    OCLContext appCLContext = {};

    appCLContext.clDevice = device;

    appCLContext.clContext = clCreateContext(propsC, 1, &device, nullptr, nullptr, &err);

    if (err != CL_SUCCESS)
    {
        std::cerr << "Failed to create CL context\n";
        return OCLContext{};
    }
    
    const cl_queue_properties propsQ[] = { 0 };
    appCLContext.clQueue = clCreateCommandQueueWithProperties(appCLContext.clContext, device, propsQ, &err);
    
    if (err != CL_SUCCESS)
    {
        std::cerr << "Failed to create CL command queue\n";
        return OCLContext{};
    }

    std::cout << "OpenCL context created successfully\n";
    return appCLContext;
}

void DestroyOCLContext(OCLContext *appContext)
{
    if (appContext->clQueue)
    {
        clFinish(appContext->clQueue);
        clReleaseCommandQueue(appContext->clQueue);
        appContext->clQueue = nullptr;
    }

    if (appContext->clContext)
    {
        clReleaseContext(appContext->clContext);
        appContext->clContext = nullptr;
    }

    appContext->clDevice = nullptr;
}

void DestroyAppContext(AppContext *appContext)
{
    if (appContext->glContext)
    {
        SDL_GL_DeleteContext(appContext->glContext);
        appContext->glContext = nullptr;
    }

    if (appContext->Window)
    {
        SDL_DestroyWindow(appContext->Window);
        appContext->Window = nullptr;
    }

    SDL_Quit();
}

struct Camera2D
{
    glm::vec2 position = {0.0f, 0.0f};
    float zoom = 1.0f;
};

bool step_dists(
    int step,
    int size,
    int &current_wf_size,
    cl::CommandQueue &queue,
    cl::Kernel &expand_wave_kernel,
    int target_idx,

    const cl::Buffer &wall_buf,

    cl::Buffer &prev_buf,
    cl::Buffer &next_buf,
    cl::Buffer &dist_buf,
    cl::Buffer &found_flag_cl,

    std::vector<int32_t> &prev_np,
    std::vector<int32_t> &next_np,
    std::vector<int32_t> &dist_np,
    std::vector<uint8_t> &found_flag_np)
{
    // Set kernel arguments
    expand_wave_kernel.setArg(0, size);
    expand_wave_kernel.setArg(1, size);
    expand_wave_kernel.setArg(2, current_wf_size);
    expand_wave_kernel.setArg(3, wall_buf);
    expand_wave_kernel.setArg(4, prev_buf);
    expand_wave_kernel.setArg(5, next_buf);
    expand_wave_kernel.setArg(6, dist_buf);
    expand_wave_kernel.setArg(7, target_idx);
    expand_wave_kernel.setArg(8, found_flag_cl);

    // Run kernel
    queue.enqueueNDRangeKernel(expand_wave_kernel, cl::NullRange,
                               cl::NDRange(current_wf_size), cl::NullRange);
    queue.enqueueReadBuffer(found_flag_cl, CL_TRUE, 0, sizeof(uint8_t), found_flag_np.data());

    if (found_flag_np[0])
    {
        std::cout << "Target found at step " << step << std::endl;
        return true;
    }

    // Reset prev buffer
    std::fill(prev_np.begin(), prev_np.end(), -1);
    queue.enqueueWriteBuffer(prev_buf, CL_TRUE, 0, sizeof(int32_t) * prev_np.size(), prev_np.data());

    // Read next wavefront
    queue.enqueueReadBuffer(next_buf, CL_TRUE, 0, sizeof(int32_t) * next_np.size(), next_np.data());

    // Count valid next elements
    std::vector<int32_t> next_pk;
    for (auto val : next_np)
        if (val != -1)
            next_pk.push_back(val);

    current_wf_size = static_cast<int>(next_pk.size());
    if (current_wf_size == 0)
    {
        std::cout << "No more cells to expand - path not found." << std::endl;
        return false;
    }

    // Copy valid_next into prev
    queue.enqueueWriteBuffer(prev_buf, CL_TRUE, 0, sizeof(int32_t) * next_pk.size(), next_pk.data());

    // Reset next buffer
    std::fill(next_np.begin(), next_np.end(), -1);
    queue.enqueueWriteBuffer(next_buf, CL_TRUE, 0, sizeof(int32_t) * next_np.size(), next_np.data());

    return false;
}

cl::Program CreateProgramFromFile(const std::string &filePath, cl::Context &context, const cl::Device &device)
{
    // Read source file
    std::string sourceCode = readFile(filePath);
    if (sourceCode.empty())
    {
        throw std::runtime_error("Error: Kernel source file is empty or could not be read.");
    }

    // Create program from source
    cl::Program::Sources sources;
    sources.push_back({sourceCode.c_str(), sourceCode.length()});

    cl::Program program(context, sources);

    // Try to build
    cl_int err = program.build({device});
    if (err != CL_SUCCESS)
    {
        std::string buildLog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
        throw std::runtime_error("Failed to build OpenCL program:\n" + buildLog);
    }

    std::cout << "OpenCL program built successfully from: " << filePath << std::endl;
    return program;
}

int main()
{
    // --- Your initialization remains unchanged ---
    const int WIN_W = 1600;
    const int WIN_H = 900;
    AppContext appCtx = CreateAppContext(WIN_W, WIN_H);
    OCLContext oclCtx = CreateOCLContext(&appCtx, 0, 0);

    float winW = static_cast<float>(appCtx.Width);
    float winH = static_cast<float>(appCtx.Height);

    // --- Load shader sources ---
    std::string vertSrc = readFile("shaders/shader.vert");
    std::string fragSrc = readFile("shaders/shader.frag");

    cl::Context context(oclCtx.clContext, true);
    cl::Device device(oclCtx.clDevice, true);
    cl::CommandQueue queue(oclCtx.clQueue, true);
    auto clProgram = CreateProgramFromFile("kernels/step_wavefront.cl", context, device);

    // --- Initialize wrappers ---
    Shader renderShader(vertSrc.c_str(), fragSrc.c_str());

    const int size = 255; // 101;
    std::string algo = "kruskal"; 
    const int worldWidth = size;
    const int worldHeight = size;
    std::vector<int32_t> maze_np = create_maze(size, algo);

    const int MAX_WF_SIZE = 2 * (std::max(size, size) - 1);
    const int INDEX_ARR_SIZE = 4 * MAX_WF_SIZE;

    auto to_ctg_idx = [size](int r, int c)
    {
        return r * size + c;
    };

    int start_idx = to_ctg_idx(1, 1);
    int target_idx = to_ctg_idx(size - 2, size - 2);

    // Initialize host buffers
    std::vector<int32_t> prev_np(INDEX_ARR_SIZE, -1);
    std::vector<int32_t> next_np(INDEX_ARR_SIZE, -1);
    std::vector<int32_t> dist_np(size * size, -1);
    std::vector<uint8_t> found_flag_np(1, 0);

    dist_np[start_idx] = 0;
    prev_np[0] = start_idx;

    // Create device buffers
    cl::Buffer wall_buf(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                        sizeof(int32_t) * maze_np.size(), (void *)maze_np.data());
    cl::Buffer prev_buf(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                        sizeof(int32_t) * prev_np.size(), (void *)prev_np.data());
    cl::Buffer next_buf(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                        sizeof(int32_t) * next_np.size(), (void *)next_np.data());
    cl::Buffer dist_buf(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                        sizeof(int32_t) * dist_np.size(), (void *)dist_np.data());
    cl::Buffer found_flag_cl(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                             sizeof(uint8_t) * found_flag_np.size(), (void *)found_flag_np.data());

    // Load kernel
    cl::Kernel expand_wave_kernel(clProgram, "expand_wave_idxs");

    SSBO wallDataBuffer(maze_np.size() * sizeof(int32_t), maze_np.data());
    SSBO distDataBuffer(dist_np.size() * sizeof(int32_t), dist_np.data());
    Quad quad;

    // --- Set uniforms ---
    renderShader.use();
    renderShader.setI("imageWidth", worldWidth);
    renderShader.setI("imageHeight", worldHeight);

    // --- Create camera ---
    auto camX = -winW / 2.0f;
    auto camY = -winH / 2.0f;
    Camera2D camera;
    camera.position = {camX, camY};
    camera.zoom = 8.0f;
    std::cout << "Camera position: " << camX << ' ' << camY << '\n';

    SDL_Event event;
    bool running = true;
    bool found = false;
    int current_size = 1; // Current wavefront size
    int current_step = 0;
    while (running)
    {
        // --- Handle SDL events ---
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = false;
            else if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_w:
                    camera.position.y += 10.0f;
                    break;
                case SDLK_s:
                    camera.position.y -= 10.0f;
                    break;
                case SDLK_a:
                    camera.position.x -= 10.0f;
                    break;
                case SDLK_d:
                    camera.position.x += 10.0f;
                    break;
                case SDLK_q:
                    camera.zoom *= 1.1f;
                    break;
                case SDLK_e:
                    camera.zoom /= 1.1f;
                    break;
                }
            }
        }
        ///*
        if (!found)
        {
            found = step_dists(current_step, size, current_size,
                               queue, expand_wave_kernel, target_idx,
                               wall_buf,
                               prev_buf, next_buf, dist_buf,
                               found_flag_cl,
                               prev_np, next_np, dist_np,
                               found_flag_np);
            // TODO: if GPU is available use GL-CL interop instead
            queue.enqueueReadBuffer(dist_buf, CL_TRUE, 0, sizeof(int32_t) * dist_np.size(), dist_np.data());
            distDataBuffer.update(sizeof(int32_t) * dist_np.size(), dist_np.data());
        }
        //*/
        // --- Compute camera matrices ---
        glm::mat4 proj = glm::ortho(0.0f, winW, 0.0f, winH);
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(-camera.position, 0.0f));
        glm::mat4 vp = proj * view * glm::scale(glm::mat4(1.0f), glm::vec3(camera.zoom, camera.zoom, 1.0f));

        // --- Render ---
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        renderShader.use();
        GLint vpLoc = glGetUniformLocation(renderShader.getID(), "vp");
        glUniformMatrix4fv(vpLoc, 1, GL_FALSE, &vp[0][0]);

        renderShader.setI("maxDist", ++current_step);

        wallDataBuffer.bind(0);
        distDataBuffer.bind(1);
        quad.draw();

        SDL_GL_SwapWindow(appCtx.Window);
    }

    // --- Cleanup ---
    DestroyOCLContext(&oclCtx);
    DestroyAppContext(&appCtx);

    return 0;
}
