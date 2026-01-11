#include "maze.h"
#include <cstdio>
#include <iostream>
#include <fstream>

namespace Maze {

std::vector<int32_t> createMaze(unsigned int size, const std::string& algorithm)
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
    for (size_t i = 0; i < mazeBytes.size(); ++i)
        mazeData[i] = static_cast<int32_t>(mazeBytes[i]);

    return mazeData;
}

} // namespace Maze
