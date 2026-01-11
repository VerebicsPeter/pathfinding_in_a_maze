#pragma once

#include <string>

namespace Utils {

/**
 * @brief Read the entire contents of a text file
 * @param filepath Path to the file to read
 * @return File contents as a string, or empty string on error
 */
std::string readFile(const std::string& filepath);

} // namespace Utils
