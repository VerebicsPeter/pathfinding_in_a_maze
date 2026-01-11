#include "cl_program.h"
#include "../utils/file_utils.h"
#include <iostream>

namespace Compute {

CLProgram::CLProgram(const std::string& filePath, cl::Context& context, cl::Device& device)
{
    // Read source file
    std::string sourceCode = Utils::readFile(filePath);
    if (sourceCode.empty())
    {
        throw std::runtime_error("Error: Kernel source file is empty or could not be read: " + filePath);
    }

    // Create program from source
    cl::Program::Sources sources;
    sources.push_back({sourceCode.c_str(), sourceCode.length()});

    m_program = cl::Program(context, sources);

    // Try to build
    cl_int buildErr = m_program.build({device});
    if (buildErr != CL_SUCCESS)
    {
        std::string buildLog = m_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
        throw std::runtime_error("Failed to build OpenCL program (Error: " + std::to_string(buildErr) + "):\n" + buildLog);
    }
    std::cout << "OpenCL program built successfully from: " << filePath << std::endl;
}

} // namespace Compute
