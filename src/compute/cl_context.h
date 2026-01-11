#pragma once

#define CL_HPP_TARGET_OPENCL_VERSION 300
#include <CL/opencl.hpp>
#include <SDL2/SDL.h>
#include <string>

namespace Compute {

/**
 * @brief OpenCL context wrapper with OpenGL interop support
 */
class CLContext
{
public:
    /**
     * @brief Create OpenCL context with GL interop
     * @param window SDL window for GL context retrieval
     * @param platformIndex OpenCL platform index (default: 0)
     * @param deviceIndex OpenCL device index (default: 0)
     */
    CLContext(SDL_Window* window, unsigned int platformIndex = 0, unsigned int deviceIndex = 0);
    ~CLContext();

    // Disable copy, allow move
    CLContext(const CLContext&) = delete;
    CLContext& operator=(const CLContext&) = delete;
    CLContext(CLContext&&) = default;
    CLContext& operator=(CLContext&&) = default;

    cl::Context& getContext() { return m_context; }
    cl::Device& getDevice() { return m_device; }
    cl::CommandQueue& getQueue() { return m_queue; }
    cl::Platform& getPlatform() { return m_platform; }

    const cl::Context& getContext() const { return m_context; }
    const cl::Device& getDevice() const { return m_device; }
    const cl::CommandQueue& getQueue() const { return m_queue; }
    const cl::Platform& getPlatform() const { return m_platform; }

private:
    cl::Platform m_platform;
    cl::Device m_device;
    cl::Context m_context;
    cl::CommandQueue m_queue;
};

} // namespace Compute
