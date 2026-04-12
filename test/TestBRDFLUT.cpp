// =============================================================================
// TestBRDFLUT - 离线生成 BRDF LUT 纹理并导出为 KTX 文件
//
// 用法: TestBRDFLUT <output_path> [imageSize] [samples]
//   output_path  输出 KTX 文件路径（如 data_asset/pbr/brdfLUT.ktx）
//   imageSize    LUT 分辨率，默认 512（必须是 2 的幂）
//   samples      Monte Carlo 采样数，默认 1024
//
// 示例:
//   TestBRDFLUT ../data_asset/pbr/brdfLUT.ktx 512 1024
//
// =============================================================================

#include "Runtime/AssetProcess/source/IBLBaker/PBRBase.h"
#include "Runtime/MathUtil/include/BitHacks.h"
#include <iostream>
#include <string>

void PrintUsage(const char* programName)
{
    std::cout << "Usage: " << programName << " <output_path> [imageSize] [samples]" << std::endl;
    std::cout << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  output_path  Path to output KTX file" << std::endl;
    std::cout << "  imageSize    LUT resolution (default: 512)" << std::endl;
    std::cout << "  samples      MC samples per pixel (default: 1024)" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << programName << " data_asset/pbr/brdfLUT.ktx 512 1024" << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Error: Output path required." << std::endl;
        PrintUsage(argv[0]);
        return 1;
    }

    // 解析参数
    std::string outputPath = argv[1];
    uint32_t imageSize = 512;     // 默认分辨率
    uint32_t samples = 1024;      // 默认采样数

    if (argc >= 3)
    {
        imageSize = static_cast<uint32_t>(std::stoi(argv[2]));
        if (!IsPowerOfTwo(imageSize))
        {
            std::cerr << "Error: imageSize must be power of 2." << std::endl;
            return 1;
        }
    }
    if (argc >= 4)
    {
        samples = static_cast<uint32_t>(std::stoi(argv[3]));
        if (samples == 0) samples = 1;
    }

    std::cout << "=== BRDF LUT Generator ===" << std::endl;
    std::cout << "Output:    " << outputPath << std::endl;
    std::cout << "Size:      " << imageSize << " x " << imageSize << std::endl;
    std::cout << "Samples:   " << samples << " / pixel" << std::endl;
    std::cout << "Format:    RG16Float (KTX 1.0)" << std::endl;
    std::cout << std::endl;

    // 调用已有的离线导出函数
    AssetProcess::GenerateBRDFLUT_Texture(outputPath, imageSize, samples);

    std::cout << "Done! BRDF LUT saved to: " << outputPath << std::endl;

    return 0;
}
