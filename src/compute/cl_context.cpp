#include "cl_context.h"
#include "../platform/platform.h"
#include <CL/cl_gl.h>
#include <iostream>
#include <vector>

namespace Compute {

CLContext::CLContext(SDL_Window* window, unsigned int platformIndex, unsigned int deviceIndex)
{
    // Get all platforms
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    if (platforms.empty())
    {
        throw std::runtime_error("No OpenCL platforms found");
    }

    std::cout << "Number of OpenCL platforms: " << platforms.size() << std::endl;

    // Print platform names
    for (size_t i = 0; i < platforms.size(); ++i)
    {
        std::string name = platforms[i].getInfo<CL_PLATFORM_NAME>();
        std::cout << "Platform " << i << ": " << name << std::endl;
    }

    // Select platform
    if (platformIndex >= platforms.size())
    {
        platformIndex = 0;
        std::cout << "Invalid platform index, using platform 0" << std::endl;
    }
    m_platform = platforms[platformIndex];

    // Setup GL interop context properties
    cl_context_properties props[7] = {0};
    
#ifdef PLATFORM_WINDOWS
    // WGL interop
    props[0] = CL_CONTEXT_PLATFORM;
    props[1] = (cl_context_properties)m_platform();
    props[2] = CL_GL_CONTEXT_KHR;
    props[3] = (cl_context_properties)wglGetCurrentContext();
    props[4] = CL_WGL_HDC_KHR;
    props[5] = (cl_context_properties)wglGetCurrentDC();
    props[6] = 0;
#elif defined(PLATFORM_LINUX)
    // EGL interop
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(window, &wmInfo))
    {
        throw std::runtime_error("Failed to get SDL window WM info");
    }

    EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(eglDisplay, nullptr, nullptr);
    EGLContext eglContext = eglGetCurrentContext();

    props[0] = CL_CONTEXT_PLATFORM;
    props[1] = (cl_context_properties)m_platform();
    props[2] = CL_GL_CONTEXT_KHR;
    props[3] = (cl_context_properties)eglContext;
    props[4] = CL_EGL_DISPLAY_KHR;
    props[5] = (cl_context_properties)eglDisplay;
    props[6] = 0;
#else
    throw std::runtime_error("Unsupported platform for GL-CL interop");
#endif

    // Get devices
    std::vector<cl::Device> devices;
    m_platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

    if (devices.empty())
    {
        throw std::runtime_error("No OpenCL devices found");
    }

    // Select device
    if (deviceIndex >= devices.size())
    {
        deviceIndex = 0;
        std::cout << "Invalid device index, using device 0" << std::endl;
    }
    m_device = devices[deviceIndex];

    std::string deviceName = m_device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Using OpenCL device: " << deviceName << std::endl;

    // Create context with properties
    m_context = cl::Context(m_device, props);

    // Create command queue
    m_queue = cl::CommandQueue(m_context, m_device);

    std::cout << "OpenCL context created successfully" << std::endl;
}

CLContext::~CLContext()
{
    // Finish any pending operations
    if (m_queue())
    {
        m_queue.finish();
    }
}

} // namespace Compute
