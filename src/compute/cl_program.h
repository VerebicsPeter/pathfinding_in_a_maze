#pragma once

#define CL_HPP_TARGET_OPENCL_VERSION 300
#include <CL/opencl.hpp>
#include <string>

namespace Compute {

/**
 * @brief OpenCL program wrapper
 */
class CLProgram
{
public:
    /**
     * @brief Create program from source file
     * @param filePath Path to .cl source file
     * @param context OpenCL context
     * @param device OpenCL device
     */
    CLProgram(const std::string& filePath, cl::Context& context, cl::Device& device);
    ~CLProgram() = default;

    // Disable copy, allow move
    CLProgram(const CLProgram&) = delete;
    CLProgram& operator=(const CLProgram&) = delete;
    CLProgram(CLProgram&&) = default;
    CLProgram& operator=(CLProgram&&) = default;

    cl::Program& getProgram() { return m_program; }
    const cl::Program& getProgram() const { return m_program; }

private:
    cl::Program m_program;
};

} // namespace Compute
