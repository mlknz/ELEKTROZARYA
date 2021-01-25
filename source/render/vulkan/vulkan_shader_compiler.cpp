#include "vulkan_shader_compiler.hpp"

#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/DirStackFileIncluder.h>
#include <glslang/Public/ShaderLang.h>

#include <fstream>

#include "core/file_utils.hpp"
#include "core/log_assert.hpp"
#include "core/scene/mesh.hpp"
#include "render/config.hpp"

namespace ez::SpirVShaderCompiler
{
namespace SDetails
{
static bool glslangInitialized = false;

static std::string GetFilePath(const std::string& str)
{
    size_t found = str.find_last_of("/\\");
    return str.substr(0, found);
    // size_t FileName = str.substr(found+1);
}

static std::string GetSuffix(const std::string& name)
{
    const size_t pos = name.rfind('.');
    return (pos == std::string::npos) ? "" : name.substr(name.rfind('.') + 1);
}

static EShLanguage GetShaderStage(const std::string& stage)
{
    if (stage == "vert") { return EShLangVertex; }
    else if (stage == "tesc")
    {
        return EShLangTessControl;
    }
    else if (stage == "tese")
    {
        return EShLangTessEvaluation;
    }
    else if (stage == "geom")
    {
        return EShLangGeometry;
    }
    else if (stage == "frag")
    {
        return EShLangFragment;
    }
    else if (stage == "comp")
    {
        return EShLangCompute;
    }
    else
    {
        assert(0 && "Unknown shader stage");
        return EShLangCount;
    }
}

constexpr int ClientInputSemanticsVersion = 100;
constexpr glslang::EShTargetClientVersion VulkanClientVersion = glslang::EShTargetVulkan_1_1;
constexpr glslang::EShTargetLanguageVersion TargetVersion = glslang::EShTargetSpv_1_3;

const TBuiltInResource DefaultTBuiltInResource = { .maxLights = 32,
                                                   .maxClipPlanes = 6,
                                                   .maxTextureUnits = 32,
                                                   .maxTextureCoords = 32,
                                                   .maxVertexAttribs = 64,
                                                   .maxVertexUniformComponents = 4096,
                                                   .maxVaryingFloats = 64,
                                                   .maxVertexTextureImageUnits = 32,
                                                   .maxCombinedTextureImageUnits = 80,
                                                   .maxTextureImageUnits = 32,
                                                   .maxFragmentUniformComponents = 4096,
                                                   .maxDrawBuffers = 32,
                                                   .maxVertexUniformVectors = 128,
                                                   .maxVaryingVectors = 8,
                                                   .maxFragmentUniformVectors = 16,
                                                   .maxVertexOutputVectors = 16,
                                                   .maxFragmentInputVectors = 15,
                                                   .minProgramTexelOffset = -8,
                                                   .maxProgramTexelOffset = 7,
                                                   .maxClipDistances = 8,
                                                   .maxComputeWorkGroupCountX = 65535,
                                                   .maxComputeWorkGroupCountY = 65535,
                                                   .maxComputeWorkGroupCountZ = 65535,
                                                   .maxComputeWorkGroupSizeX = 1024,
                                                   .maxComputeWorkGroupSizeY = 1024,
                                                   .maxComputeWorkGroupSizeZ = 64,
                                                   .maxComputeUniformComponents = 1024,
                                                   .maxComputeTextureImageUnits = 16,
                                                   .maxComputeImageUniforms = 8,
                                                   .maxComputeAtomicCounters = 8,
                                                   .maxComputeAtomicCounterBuffers = 1,
                                                   .maxVaryingComponents = 60,
                                                   .maxVertexOutputComponents = 64,
                                                   .maxGeometryInputComponents = 64,
                                                   .maxGeometryOutputComponents = 128,
                                                   .maxFragmentInputComponents = 128,
                                                   .maxImageUnits = 8,
                                                   .maxCombinedImageUnitsAndFragmentOutputs = 8,
                                                   .maxCombinedShaderOutputResources = 8,
                                                   .maxImageSamples = 0,
                                                   .maxVertexImageUniforms = 0,
                                                   .maxTessControlImageUniforms = 0,
                                                   .maxTessEvaluationImageUniforms = 0,
                                                   .maxGeometryImageUniforms = 0,
                                                   .maxFragmentImageUniforms = 8,
                                                   .maxCombinedImageUniforms = 8,
                                                   .maxGeometryTextureImageUnits = 16,
                                                   .maxGeometryOutputVertices = 256,
                                                   .maxGeometryTotalOutputComponents = 1024,
                                                   .maxGeometryUniformComponents = 1024,
                                                   .maxGeometryVaryingComponents = 64,
                                                   .maxTessControlInputComponents = 128,
                                                   .maxTessControlOutputComponents = 128,
                                                   .maxTessControlTextureImageUnits = 16,
                                                   .maxTessControlUniformComponents = 1024,
                                                   .maxTessControlTotalOutputComponents = 4096,
                                                   .maxTessEvaluationInputComponents = 128,
                                                   .maxTessEvaluationOutputComponents = 128,
                                                   .maxTessEvaluationTextureImageUnits = 16,
                                                   .maxTessEvaluationUniformComponents = 1024,
                                                   .maxTessPatchComponents = 120,
                                                   .maxPatchVertices = 32,
                                                   .maxTessGenLevel = 64,
                                                   .maxViewports = 16,
                                                   .maxVertexAtomicCounters = 0,
                                                   .maxTessControlAtomicCounters = 0,
                                                   .maxTessEvaluationAtomicCounters = 0,
                                                   .maxGeometryAtomicCounters = 0,
                                                   .maxFragmentAtomicCounters = 8,
                                                   .maxCombinedAtomicCounters = 8,
                                                   .maxAtomicCounterBindings = 1,
                                                   .maxVertexAtomicCounterBuffers = 0,
                                                   .maxTessControlAtomicCounterBuffers = 0,
                                                   .maxTessEvaluationAtomicCounterBuffers = 0,
                                                   .maxGeometryAtomicCounterBuffers = 0,
                                                   .maxFragmentAtomicCounterBuffers = 1,
                                                   .maxCombinedAtomicCounterBuffers = 1,
                                                   .maxAtomicCounterBufferSize = 16384,
                                                   .maxTransformFeedbackBuffers = 4,
                                                   .maxTransformFeedbackInterleavedComponents =
                                                       64,
                                                   .maxCullDistances = 8,
                                                   .maxCombinedClipAndCullDistances = 8,
                                                   .maxSamples = 4,
                                                   .maxMeshOutputVerticesNV = 256,
                                                   .maxMeshOutputPrimitivesNV = 512,
                                                   .maxMeshWorkGroupSizeX_NV = 32,
                                                   .maxMeshWorkGroupSizeY_NV = 1,
                                                   .maxMeshWorkGroupSizeZ_NV = 1,
                                                   .maxTaskWorkGroupSizeX_NV = 32,
                                                   .maxTaskWorkGroupSizeY_NV = 1,
                                                   .maxTaskWorkGroupSizeZ_NV = 1,
                                                   .maxMeshViewCountNV = 4,
                                                   .limits = {
                                                       .nonInductiveForLoops = true,
                                                       .whileLoops = true,
                                                       .doWhileLoops = true,
                                                       .generalUniformIndexing = true,
                                                       .generalAttributeMatrixVectorIndexing =
                                                           true,
                                                       .generalVaryingIndexing = true,
                                                       .generalSamplerIndexing = true,
                                                       .generalVariableIndexing = true,
                                                       .generalConstantMatrixVectorIndexing =
                                                           true,
                                                   } };
}  // namespace SDetails

// todo: file loading codes. std::vector<char> InputGLSL = FileUtils::ReadFile(filename);

// todo: Save compiled .spv to FS and load if shader hash didn't
// change) glslang::OutputSpvBin(SpirV, (GetSuffix(filename) +
// std::string(".spv")).c_str());

const std::vector<uint32_t> CompileFromGLSL(const std::string& filename)
{
    using namespace SDetails;

    if (!glslangInitialized)
    {
        glslang::InitializeProcess();
        glslangInitialized = true;
    }

    std::ifstream file(filename);

    if (!file.is_open())
    {
        std::cout << "Failed to load shader: " << filename << std::endl;
        throw std::runtime_error("failed to open file: " + filename);
    }

    std::string InputGLSL((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());

    const char* InputCString = InputGLSL.data();

    EShLanguage ShaderType = GetShaderStage(GetSuffix(filename));
    glslang::TShader Shader(ShaderType);
    Shader.setStrings(&InputCString, 1);

    Shader.setEnvInput(glslang::EShSourceGlsl,
                       ShaderType,
                       glslang::EShClientVulkan,
                       ClientInputSemanticsVersion);
    Shader.setEnvClient(glslang::EShClientVulkan, VulkanClientVersion);
    Shader.setEnvTarget(glslang::EShTargetSpv, TargetVersion);

    TBuiltInResource Resources;
    Resources = DefaultTBuiltInResource;
    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    const int DefaultVersion = 100;

    DirStackFileIncluder Includer;

    std::string Path = GetFilePath(filename);
    Includer.pushExternalLocalDirectory(Path);

    std::string PreprocessedGLSL;

    if (!Shader.preprocess(&Resources,
                           DefaultVersion,
                           ENoProfile,
                           false,
                           false,
                           messages,
                           &PreprocessedGLSL,
                           Includer))
    {
        EZLOG("GLSL Preprocessing Failed for: ", filename);
        EZLOG(Shader.getInfoLog());
        EZLOG(Shader.getInfoDebugLog());
    }

    if (Config::dumpGlslSources) { EZLOG(PreprocessedGLSL); }

    const char* PreprocessedCStr = PreprocessedGLSL.c_str();
    Shader.setStrings(&PreprocessedCStr, 1);

    if (!Shader.parse(&Resources, 100, false, messages))
    {
        EZLOG("GLSL Parsing Failed for: ", filename);
        EZLOG(Shader.getInfoLog());
        EZLOG(Shader.getInfoDebugLog());
    }

    glslang::TProgram Program;
    Program.addShader(&Shader);

    if (!Program.link(messages))
    {
        EZLOG("GLSL Linking Failed for:", filename);
        EZLOG(Shader.getInfoLog());
        EZLOG(Shader.getInfoDebugLog());
    }

    std::vector<uint32_t> SpirV;
    spv::SpvBuildLogger logger;
    glslang::SpvOptions spvOptions;
    glslang::GlslangToSpv(*Program.getIntermediate(ShaderType), SpirV, &logger, &spvOptions);

    if (logger.getAllMessages().length() > 0) { EZLOG(logger.getAllMessages()); }

    return SpirV;
}
}  // namespace ez::SpirVShaderCompiler
