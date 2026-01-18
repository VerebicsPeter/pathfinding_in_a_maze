#include "maze.h"
#include <cstdio>
#include <iostream>
#include <fstream>
#include <random>

namespace Maze {

std::vector<int32_t> createMaze(unsigned int size, const std::string& algorithm, bool randomCost)
{
    char cmd[256];
    std::snprintf(cmd, sizeof(cmd), "python gen_maze.py %u \"%s\"", size, algorithm.c_str());
    int ret = std::system(cmd);
    if (ret != 0)
    {
        std::cerr << "Error while executing python script." << std::endl;
        return {};
    }

    std::vector<uint8_t> mazeBytes(size * size);

    std::ifstream file("maze.bin", std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open maze.bin" << std::endl;
        return {};
    }
    
    file.read(reinterpret_cast<char*>(mazeBytes.data()), mazeBytes.size());
    file.close();

    std::cout << "Generated maze." << std::endl;

    std::vector<int32_t> mazeData(mazeBytes.size());
    
    // Setup random number generation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int32_t> dist(1, 255);

    uint8_t mazeByte;
    for (size_t i = 0; i < mazeBytes.size(); ++i)
    {
        mazeByte = mazeBytes[i];
        if (mazeByte == 1)
        {   // NOTE: we use negative values to represent walls
            mazeData[i] = -1;
        }
        else
        {
            mazeData[i] = randomCost ? dist(gen) : 1;
        }
    }

    return mazeData;
}

bool initializeMazeState(
    const Compute::CLContext& clContext,
    const Compute::CLProgram& clProgram,
    const std::vector<int32_t>& hostMazeCosts,
    int mazeSize, int startIndex,
    MazeState& mazeState
)
{
    auto& st = mazeState;
    // Initialize buffers
    // Wall buffer is already generated in initMaze
    
    const int maxWfSize = 2 * (std::max(mazeSize, mazeSize) - 1);
    const int indexArrSize = 4 * maxWfSize;

    st.prevHost.resize(indexArrSize, -1);
    st.nextHost.resize(indexArrSize, -1);
    st.distHost.resize(mazeSize * mazeSize, -1);
    st.visitedFlag.resize(mazeSize * mazeSize, 0);
    st.foundFlagHost.resize(1, 0);

    st.distHost[startIndex] = 0;
    st.prevHost[0] = startIndex;

    // Create OpenCL buffers
    st.costBuf = cl::Buffer(
        clContext.getContext(),
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(int32_t) * hostMazeCosts.size(),
        const_cast<int32_t*> (hostMazeCosts.data()) /// cast away const...
    );

    st.prevBuf = cl::Buffer(
        clContext.getContext(),
        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        sizeof(int32_t) * st.prevHost.size(),
        st.prevHost.data()
    );

    st.nextBuf = cl::Buffer(
        clContext.getContext(),
        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        sizeof(int32_t) * st.nextHost.size(),
        st.nextHost.data()
    );

    st.distBuf = cl::Buffer(
        clContext.getContext(),
        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        sizeof(int32_t) * st.distHost.size(),
        st.distHost.data()
    );

    st.foundFlagBuf = cl::Buffer(
        clContext.getContext(),
        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        sizeof(uint8_t) * st.foundFlagHost.size(),
        st.foundFlagHost.data()
    );

    // Create kernel
    st.kernel = cl::Kernel(clProgram.getProgram(), "expand_wave_idxs");

    return true;
}

} // namespace Maze
