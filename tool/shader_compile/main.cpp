#include "Runtime/AssetProcess/include/ShaderImporter.h"
#include "Runtime/ShaderCompiler/include/ShaderCompiler.h"
#include "Runtime/ShaderCompiler/include/ShaderCompilerDefine.h"
#include "Runtime/AssetManager/include/ShaderMessage.pb.h"
#include <iostream>
#include <string>
#include <cstring>

// ====================================================================
// shader_compile — 离线 shader 编译工具
//
// 用法:
//   shader_compile <input.hlsl> -s vs|ps|cs|ts|ms -f spirv|msl_ios|msl_macos|dxil|glsl -o <output.gnxasset>
//
// 示例:
//   shader_compile pbr_vs.hlsl -s vs -f msl_ios -o ../data_asset/shaders/pbr_vs.ios.gnxasset
//   shader_compile pbr_vs.hlsl -s vs -f spirv   -o ../data_asset/shaders/pbr_vs.android.gnxasset
// ====================================================================

static void PrintUsage(const char* exeName)
{
    std::cout << "用法:" << std::endl;
    std::cout << "  " << exeName << " <input.hlsl> -s <stage> -f <format> [-o <output>]" << std::endl;
    std::cout << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  -s   shader 阶段: vs | ps | cs | ts | ms" << std::endl;
    std::cout << "  -f   目标格式:    spirv | msl_ios | msl_macos | dxil | glsl" << std::endl;
    std::cout << "  -o   输出文件     默认: <input>.gnxasset" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  " << exeName << " pbr_vs.hlsl -s vs -f msl_ios -o pbr_vs.ios.gnxasset" << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 5)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    std::string inputPath;
    RenderCore::ShaderStage stage = RenderCore::ShaderStage_Vertex;
    uint32_t format = GnxShaderFormat_GnxShaderFormat_SPIRV;
    std::string outputPath;

    // 解析参数
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc)
        {
            std::string s = argv[++i];
            if (s == "vs")      stage = RenderCore::ShaderStage_Vertex;
            else if (s == "ps") stage = RenderCore::ShaderStage_Fragment;
            else if (s == "cs") stage = RenderCore::ShaderStage_Compute;
            else if (s == "ts") stage = RenderCore::ShaderStage_Task;
            else if (s == "ms") stage = RenderCore::ShaderStage_Mesh;
            else { std::cerr << "未知 shader 阶段: " << s << std::endl; return 1; }
        }
        else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc)
        {
            std::string f = argv[++i];
            if (f == "spirv")        format = GnxShaderFormat_GnxShaderFormat_SPIRV;
            else if (f == "msl_ios") format = GnxShaderFormat_GnxShaderFormat_MSL_iOS;
            else if (f == "msl_macos") format = GnxShaderFormat_GnxShaderFormat_MSL_macOS;
            else if (f == "dxil")    format = GnxShaderFormat_GnxShaderFormat_DXIL;
            else if (f == "glsl")    format = GnxShaderFormat_GnxShaderFormat_GLSL;
            else { std::cerr << "未知目标格式: " << f << std::endl; return 1; }
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
        std::cerr << "错误: 未指定输入文件" << std::endl;
        return 1;
    }

    if (outputPath.empty())
    {
        // 默认输出名
        outputPath = inputPath + ".gnxasset";
    }

    std::cout << "=== Shader 离线编译 ===" << std::endl;
    std::cout << "  输入:  " << inputPath << std::endl;
    std::cout << "  目标格式: " << format << std::endl;
    std::cout << "  输出:  " << outputPath << std::endl;

    AssetProcess::ShaderImporter importer;
    importer.SetSourcePath(inputPath);
    importer.SetShaderStage(static_cast<uint32_t>(stage));
    importer.SetTargetFormat(format);
    importer.SetOutputPath(outputPath);

    if (!importer.ImportAndSave())
    {
        std::cerr << "编译失败!" << std::endl;
        return 1;
    }

    std::cout << "编译成功: " << outputPath << std::endl;
    return 0;
}
