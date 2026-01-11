#include "pathfinding.h"
#include <algorithm>
#include <iostream>

namespace Maze {

bool stepPathfinding(
    int step,
    int size,
    int& currentWfSize,
    cl::CommandQueue& queue,
    cl::Kernel& kernel,
    int targetIdx,
    const cl::Buffer& wallBuf,
    cl::Buffer& prevBuf,
    cl::Buffer& nextBuf,
    cl::Buffer& distBuf,
    cl::Buffer& foundFlagBuf,
    std::vector<int32_t>& prevHost,
    std::vector<int32_t>& nextHost,
    std::vector<int32_t>& distHost,
    std::vector<uint8_t>& foundFlagHost
)
{
    // Set kernel arguments
    kernel.setArg(0, size);
    kernel.setArg(1, size);
    kernel.setArg(2, currentWfSize);
    kernel.setArg(3, wallBuf);
    kernel.setArg(4, prevBuf);
    kernel.setArg(5, nextBuf);
    kernel.setArg(6, distBuf);
    kernel.setArg(7, targetIdx);
    kernel.setArg(8, foundFlagBuf);

    // Run kernel
    queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(currentWfSize), cl::NullRange);
    queue.enqueueReadBuffer(foundFlagBuf, CL_TRUE, 0, sizeof(uint8_t), foundFlagHost.data());

    if (foundFlagHost[0])
    {
        std::cout << "Target found at step " << step << std::endl;
        return true;
    }

    // Reset prev buffer
    std::fill(prevHost.begin(), prevHost.end(), -1);
    queue.enqueueWriteBuffer(prevBuf, CL_TRUE, 0, sizeof(int32_t) * prevHost.size(), prevHost.data());

    // Read next wavefront
    queue.enqueueReadBuffer(nextBuf, CL_TRUE, 0, sizeof(int32_t) * nextHost.size(), nextHost.data());

    // Count valid next elements
    std::vector<int32_t> nextPacked;
    for (auto val : nextHost)
        if (val != -1)
            nextPacked.push_back(val);

    currentWfSize = static_cast<int>(nextPacked.size());
    if (currentWfSize == 0)
    {
        std::cout << "No more cells to expand - path not found." << std::endl;
        return false;
    }

    // Copy valid next into prev
    queue.enqueueWriteBuffer(prevBuf, CL_TRUE, 0, sizeof(int32_t) * nextPacked.size(), nextPacked.data());

    // Reset next buffer
    std::fill(nextHost.begin(), nextHost.end(), -1);
    queue.enqueueWriteBuffer(nextBuf, CL_TRUE, 0, sizeof(int32_t) * nextHost.size(), nextHost.data());

    return false;
}

} // namespace Maze
