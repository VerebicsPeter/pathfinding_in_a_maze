#pragma once

#define CL_HPP_TARGET_OPENCL_VERSION 300
#include <CL/opencl.hpp>

#include <vector>
#include <cstdint>
#include <string>

#include "../compute/cl_context.h"
#include "../compute/cl_program.h"


namespace Maze {

/**
 * @brief Create a maze using Python script
 * @param size Size of the maze (width and height)
 * @param algorithm Algorithm to use ("kruskal" or "depthfs")
 * @return Vector of cost data (negative = wall, positive = cost this is between 1 and 255)
 */
std::vector<int32_t> createMaze(unsigned int size, const std::string& algorithm, bool randomCost = false);

struct MazeState {
    cl::Buffer costBuf;
    cl::Buffer prevBuf;
    cl::Buffer nextBuf;
    cl::Buffer distBuf;
    cl::Buffer foundFlagBuf;

    std::vector<int32_t> prevHost;
    std::vector<int32_t> nextHost;
    std::vector<int32_t> distHost;
    std::vector<int32_t> visitedFlag; /// flag for backtracking state
    std::vector<uint8_t> foundFlagHost;

    cl::Kernel kernel;
};

bool initializeMazeState(
    const Compute::CLContext& clContext,
    const Compute::CLProgram& clProgram,
    const std::vector<int32_t>& hostMazeCosts,
    int mazeSize, int startIndex,
    MazeState& mazeState
);
} // namespace Maze
