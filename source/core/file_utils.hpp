#pragma once

#include <string>
#include <vector>

namespace ez::FileUtils
{
std::vector<char> ReadFile(const std::string& filename);
}
