#include "Runtime/ShaderCompiler/include/ShaderCompiler.h"
#include "Runtime/ShaderCompiler/include/ShaderCompilerDefine.h"
#include "Runtime/AssetManager/include/ShaderPackageBuilder.h"
#include "Runtime/RenderCore/include/ShaderStageData.h"
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <fstream>
#include <sstream>
#include <regex>

// ====================================================================
// shader_compile - offline shader compile tool (container format)
//
// Usage:
//   shader_compile <input.shader> -s vs|ps|cs|ts|ms -f <format> [-o <output.gnxasset>]
//   shader_compile <input.shader> -a -f <format> [-o <output.gnxasset>]   # all stages
//
// Output: single {name}.{format}.gnxasset (ShaderPackageMessage container with all stages)
//
// Examples:
//   shader_compile GBufferPBR.shader -a -f spirv -o data_asset/Shader/GBufferPBR.spirv.gnxasset
//   shader_compile GBufferPBR.shader -s vs -f spirv -o data_asset/Shader/GBufferPBR.spirv.gnxasset
// ====================================================================

static void PrintUsage(const char* exeName)
{
    std::cout << "Usage:" << std::endl;
    std::cout << "  " << exeName << " <input.shader> -s <stage> -f <format> [-o <output>]" << std::endl;
    std::cout << "  " << exeName << " <input.shader> -a -f <format> [-o <output>]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -s   shader stage: vs | ps | cs | ts | ms" << std::endl;
    std::cout << "  -a   compile all stages (mutually exclusive with -s)" << std::endl;
    std::cout << "  -f   target format: spirv | msl_ios | msl_macos | dxil | glsl" << std::endl;
    std::cout << "  -o   output file    default: <input>.<format>.gnxasset" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << exeName << " GBufferPBR.shader -a -f spirv -o GBufferPBR.spirv.gnxasset" << std::endl;
}

// Parse format string -> ShaderFormat
static bool ParseFormat(const std::string& f, RenderCore::ShaderFormat& out)
{
    if (f == "spirv")         out = RenderCore::ShaderFormat_SPIRV;
    else if (f == "msl_ios")  out = RenderCore::ShaderFormat_MSL_iOS;
    else if (f == "msl_macos") out = RenderCore::ShaderFormat_MSL_macOS;
    else if (f == "dxil")     out = RenderCore::ShaderFormat_DXIL;
    else if (f == "glsl")     out = RenderCore::ShaderFormat_GLSL;
    else return false;
    return true;
}

// Parse stage string -> ShaderStage
static bool ParseStage(const std::string& s, RenderCore::ShaderStage& out)
{
    if (s == "vs")      out = RenderCore::ShaderStage_Vertex;
    else if (s == "ps") out = RenderCore::ShaderStage_Fragment;
    else if (s == "cs") out = RenderCore::ShaderStage_Compute;
    else if (s == "ts") out = RenderCore::ShaderStage_Task;
    else if (s == "ms") out = RenderCore::ShaderStage_Mesh;
    else return false;
    return true;
}

// Stage -> entry point function name (Metal needs this)
static const char* StageToEntryPoint(RenderCore::ShaderStage stage)
{
    switch (stage)
    {
        case RenderCore::ShaderStage_Vertex:   return "VS";
        case RenderCore::ShaderStage_Fragment: return "PS";
        case RenderCore::ShaderStage_Compute:  return "CS";
        case RenderCore::ShaderStage_Task:     return "TS";
        case RenderCore::ShaderStage_Mesh:     return "MS";
        default:                               return "VS";
    }
}

// Detect entry stages in a .shader file (same rule as compile_shaders.ps1/.sh)
// Matches VS(/PS(/CS(/TS(/MS( preceded by line start or whitespace, covering:
//   void CS( ...          -> CS
//   VertexOut VS(...)     -> VS
//   VS_OUTPUT VS(...)     -> VS
static void DetectStages(const std::string& content, std::vector<RenderCore::ShaderStage>& out)
{
    std::regex re(R"((?:^|\s)(VS|PS|CS|TS|MS)\s*\()");
    auto begin = std::sregex_iterator(content.begin(), content.end(), re);
    auto end = std::sregex_iterator();
    bool has[5] = { false, false, false, false, false };
    for (auto it = begin; it != end; ++it)
    {
        std::string m = (*it)[1].str();
        if (m == "VS") has[0] = true;
        else if (m == "PS") has[1] = true;
        else if (m == "CS") has[2] = true;
        else if (m == "TS") has[3] = true;
        else if (m == "MS") has[4] = true;
    }
    if (has[0]) out.push_back(RenderCore::ShaderStage_Vertex);
    if (has[1]) out.push_back(RenderCore::ShaderStage_Fragment);
    if (has[2]) out.push_back(RenderCore::ShaderStage_Compute);
    if (has[3]) out.push_back(RenderCore::ShaderStage_Task);
    if (has[4]) out.push_back(RenderCore::ShaderStage_Mesh);
}

int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    std::string inputPath;
    std::string outputPath;
    RenderCore::ShaderFormat format = RenderCore::ShaderFormat_SPIRV;
    RenderCore::ShaderStage singleStage = RenderCore::ShaderStage_Vertex;
    bool allStages = false;
    bool stageSpecified = false;

    // Parse args
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc)
        {
            if (!ParseStage(argv[++i], singleStage))
            {
                std::cerr << "Unknown shader stage: " << argv[i] << std::endl;
                return 1;
            }
            stageSpecified = true;
        }
        else if (strcmp(argv[i], "-a") == 0)
        {
            allStages = true;
        }
        else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc)
        {
            if (!ParseFormat(argv[++i], format))
            {
                std::cerr << "Unknown target format: " << argv[i] << std::endl;
                return 1;
            }
        }
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
        {
            outputPath = argv[++i];
        }
        else if (argv[i][0] != '-')
        {
            inputPath = argv[i];
        }
    }

    if (inputPath.empty())
    {
        std::cerr << "ERROR: No input file specified" << std::endl;
        return 1;
    }

    // -a and -s are mutually exclusive
    if (allStages && stageSpecified)
    {
        std::cerr << "ERROR: -a and -s are mutually exclusive" << std::endl;
        return 1;
    }

    // Read .shader source file
    std::ifstream srcFile(inputPath, std::ios::binary);
    if (!srcFile.is_open())
    {
        std::cerr << "ERROR: cannot open input: " << inputPath << std::endl;
        return 1;
    }
    std::stringstream ss;
    ss << srcFile.rdbuf();
    std::string content = ss.str();
    srcFile.close();

    // Compute source hash (FNV-1a 64, for incremental compile / version tracking)
    uint64_t sourceHash = 0;
    {
        const uint8_t* data = (const uint8_t*)content.data();
        size_t size = content.size();
        uint64_t h = 1469598103934665603ULL;
        for (size_t i = 0; i < size; ++i)
        {
            h ^= data[i];
            h *= 1099511628211ULL;
        }
        sourceHash = h;
    }

    // Determine stages to compile
    std::vector<RenderCore::ShaderStage> stages;
    if (allStages)
    {
        DetectStages(content, stages);
        if (stages.empty())
        {
            std::cerr << "ERROR: no VS/PS/CS/TS/MS entry detected in " << inputPath << std::endl;
            return 1;
        }
    }
    else
    {
        stages.push_back(singleStage);
    }

    // Default output name: {input}.{format}.gnxasset
    if (outputPath.empty())
    {
        outputPath = inputPath + "." + RenderCore::ShaderFormatToString(format) + ".gnxasset";
    }

    std::cout << "=== Shader Offline Compilation ===" << std::endl;
    std::cout << "  Input:  " << inputPath << std::endl;
    std::cout << "  Format: " << RenderCore::ShaderFormatToString(format) << std::endl;
    std::cout << "  Stages: " << stages.size() << std::endl;
    std::cout << "  Output: " << outputPath << std::endl;

    // Assemble via ShaderPackageBuilder (no pb exposure)
    AssetManager::ShaderPackageBuilder builder;
    builder.SetFormat(format);

    // shaderName from output path: {dir}/{name}.{format}.gnxasset -> {dir}/{name}
    // (relative to data_asset/Shader, keeps subdir prefix like "vt/GBufferVTPBR")
    {
        std::string name = outputPath;
        // strip ".{format}.gnxasset" suffix
        std::string suffix = std::string(".") + RenderCore::ShaderFormatToString(format) + ".gnxasset";
        if (name.size() > suffix.size() &&
            name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0)
        {
            name = name.substr(0, name.size() - suffix.size());
        }
        else
        {
            auto ext = name.rfind(".gnxasset");
            if (ext != std::string::npos)
                name = name.substr(0, ext);
        }
        builder.SetShaderName(name);
    }

    for (const auto& stage : stages)
    {
        std::cout << "  [compile] stage " << StageToEntryPoint(stage)
                  << " -> " << RenderCore::ShaderFormatToString(format) << std::endl;

        shader_compiler::CompiledShaderInfoPtr result =
            shader_compiler::CompileShader(inputPath, stage, format);
        if (!result || !result->shaderSource || result->shaderSource->empty())
        {
            std::cerr << "ERROR: compile failed for stage "
                      << StageToEntryPoint(stage) << ": " << inputPath << std::endl;
            return 1;
        }

        builder.AddStage(stage,
                         StageToEntryPoint(stage),
                         *result->shaderSource,
                         result->vertexDescriptor,
                         result->threadgroupSizeX,
                         result->threadgroupSizeY,
                         result->threadgroupSizeZ,
                         result->pushConstants,
                         sourceHash);
    }

    if (!builder.Save(outputPath))
    {
        std::cerr << "Compilation failed!" << std::endl;
        return 1;
    }

    std::cout << "Compilation succeeded: " << outputPath << std::endl;
    return 0;
}
