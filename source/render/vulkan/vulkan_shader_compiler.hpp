#pragma once

#include <string>
#include <vector>

namespace ez::SpirVShaderCompiler
{
// https://forestsharp.com/glslang-cpp/
// https://github.com/ForestCSharp/VkCppRenderer/blob/master/Src/Renderer/GLSL/ShaderCompiler.hpp

const std::vector<uint32_t> CompileFromGLSL(const std::string& filename);

}  // namespace ez::SpirVShaderCompiler
