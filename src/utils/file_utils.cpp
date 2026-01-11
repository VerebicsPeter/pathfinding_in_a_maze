#include "file_utils.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace Utils {

std::string readFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace Utils
