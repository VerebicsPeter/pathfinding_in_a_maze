#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace Maze {

/**
 * @brief Maze data structure
 */
struct MazeData
{
    unsigned int width;
    unsigned int height;
    std::vector<int32_t> walls;
    std::vector<int32_t> distances;
};

/**
 * @brief Create a maze using Python script
 * @param size Size of the maze (width and height)
 * @param algorithm Algorithm to use ("kruskal" or "depthfs")
 * @return Vector of wall data (1 = wall, 0 = empty)
 */
std::vector<int32_t> createMaze(unsigned int size, const std::string& algorithm);

} // namespace Maze
