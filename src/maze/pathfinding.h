#pragma once

#define CL_HPP_TARGET_OPENCL_VERSION 300
#include <CL/opencl.hpp>
#include <vector>
#include <cstdint>

namespace Maze {

/**
 * @brief Execute one step of wavefront pathfinding
 * @param step Current step number
 * @param size Maze size
 * @param currentWfSize Current wavefront size (will be updated)
 * @param queue OpenCL command queue
 * @param kernel OpenCL kernel for wavefront expansion
 * @param targetIdx Target cell index
 * @param costBuf OpenCL buffer containing wall data
 * @param prevBuf OpenCL buffer for previous wavefront
 * @param nextBuf OpenCL buffer for next wavefront
 * @param distBuf OpenCL buffer for distance data
 * @param foundFlagBuf OpenCL buffer for found flag
 * @param prevHost Host buffer for previous wavefront
 * @param nextHost Host buffer for next wavefront
 * @param distHost Host buffer for distances
 * @param foundFlagHost Host buffer for found flag
 * @return true if target found, false otherwise
 */
bool stepPathfinding(
    int step,
    int size,
    int& currentWfSize,
    cl::CommandQueue& queue,
    cl::Kernel& kernel,
    int targetIdx,
    const cl::Buffer& costBuf,
    cl::Buffer& prevBuf,
    cl::Buffer& nextBuf,
    cl::Buffer& distBuf,
    cl::Buffer& foundFlagBuf,
    std::vector<int32_t>& prevHost,
    std::vector<int32_t>& nextHost,
    std::vector<int32_t>& distHost,
    std::vector<uint8_t>& foundFlagHost
);

} // namespace Maze
