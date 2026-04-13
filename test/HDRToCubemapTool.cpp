/**
 * HDRToCubemapTool - Integrated HDR environment map to Cubemap conversion tool
 *
 * Usage: HDRToCubemapTool <input_hdr> [options]
 *
 * Options:
 *   --output-dir <dir>        Output directory (default: same as input)
 *   --cubemap-size <size>     Cubemap face size (default: 512)
 *   --irradiance-size <size>  Irradiance map face size (default: 32)
 *   --prefilter-size <size>   Prefiltered env map face size (default: 128)
 *   --samples <num>           Samples for IBL generation (default: 256)
 *   --no-ibl                  Skip IBL generation (only cubemap KTX)
 *   --no-prefilter            Skip prefiltered env map generation
 *   --png                     Also save cubemap faces as PNG (tone-mapped)
 *   --help                    Show this help message
 *
 * Output files:
 *   <basename>_cubemap.ktx          HDR Cubemap (RGB32Float, with mipmaps)
 *   <basename>_irradiance.ktx       Diffuse irradiance map
 *   <basename>_prefilter.ktx        Prefiltered environment map
 *   <basename>_face_N.png           Individual face PNGs (if --png)
 */

#include "Runtime/AssetProcess/source/TextureProcess/EnvHdrProcess.h"
#include "Runtime/AssetProcess/source/IBLBaker/PBRBase.h"
#include "Runtime/AssetProcess/include/TextureImporter.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "Runtime/ImageCodec/include/ImageEncoder.h"
#include "Runtime/MathUtil/include/MathUtil.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

struct ToolOptions
{
    std::string inputFile;
    std::string outputDir;
    uint32_t cubemapSize = 512;
    uint32_t irradianceSize = 32;
    uint32_t prefilterSize = 128;
    uint32_t samples = 256;
    bool generateIBL = true;
    bool generatePrefilter = true;
    bool savePNG = false;
};

void PrintUsage(const char* programName)
{
    std::cout << "HDRToCubemapTool - HDR Environment Map to Cubemap Converter" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: " << programName << " <input_hdr> [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --output-dir <dir>        Output directory (default: same as input)" << std::endl;
    std::cout << "  --cubemap-size <size>     Cubemap face size (default: 512)" << std::endl;
    std::cout << "  --irradiance-size <size>  Irradiance map face size (default: 32)" << std::endl;
    std::cout << "  --prefilter-size <size>   Prefiltered env map face size (default: 128)" << std::endl;
    std::cout << "  --samples <num>           Samples for IBL generation (default: 256)" << std::endl;
    std::cout << "  --no-ibl                  Skip IBL generation (only cubemap KTX)" << std::endl;
    std::cout << "  --no-prefilter            Skip prefiltered env map generation" << std::endl;
    std::cout << "  --png                     Also save cubemap faces as PNG (tone-mapped)" << std::endl;
    std::cout << "  --help                    Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Output files:" << std::endl;
    std::cout << "  <basename>_cubemap.ktx          HDR Cubemap" << std::endl;
    std::cout << "  <basename>_irradiance.ktx       Diffuse irradiance map" << std::endl;
    std::cout << "  <basename>_prefilter.ktx        Prefiltered environment map" << std::endl;
    std::cout << "  <basename>_face_N.png           Individual face PNGs (if --png)" << std::endl;
}

bool ParseArgs(int argc, char* argv[], ToolOptions& opts)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h")
        {
            PrintUsage(argv[0]);
            return false;
        }
        else if (arg == "--output-dir" && i + 1 < argc)
        {
            opts.outputDir = argv[++i];
        }
        else if (arg == "--cubemap-size" && i + 1 < argc)
        {
            opts.cubemapSize = std::stoi(argv[++i]);
        }
        else if (arg == "--irradiance-size" && i + 1 < argc)
        {
            opts.irradianceSize = std::stoi(argv[++i]);
        }
        else if (arg == "--prefilter-size" && i + 1 < argc)
        {
            opts.prefilterSize = std::stoi(argv[++i]);
        }
        else if (arg == "--samples" && i + 1 < argc)
        {
            opts.samples = std::stoi(argv[++i]);
        }
        else if (arg == "--no-ibl")
        {
            opts.generateIBL = false;
        }
        else if (arg == "--no-prefilter")
        {
            opts.generatePrefilter = false;
        }
        else if (arg == "--png")
        {
            opts.savePNG = true;
        }
        else if (arg[0] != '-')
        {
            if (opts.inputFile.empty())
            {
                opts.inputFile = arg;
            }
            else
            {
                std::cerr << "Error: Unexpected argument: " << arg << std::endl;
                return false;
            }
        }
        else
        {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            return false;
        }
    }

    if (opts.inputFile.empty())
    {
        std::cerr << "Error: No input HDR file specified." << std::endl;
        PrintUsage(argv[0]);
        return false;
    }

    if (opts.outputDir.empty())
    {
        opts.outputDir = fs::path(opts.inputFile).parent_path().string();
    }

    return true;
}

imagecodec::VImagePtr ConvertHDRToRGBA8(const imagecodec::VImage* srcImage)
{
    if (!srcImage || !srcImage->GetImageData())
    {
        return nullptr;
    }

    imagecodec::VImagePtr result = std::make_shared<imagecodec::VImage>();
    result->SetImageInfo(imagecodec::FORMAT_RGBA8, srcImage->GetWidth(), srcImage->GetHeight());
    result->AllocPixels();

    float* pSrc = (float*)srcImage->GetImageData();
    uint8_t* pDst = (uint8_t*)result->GetImageData();
    uint32_t offset = 0;

    for (uint32_t i = 0; i < srcImage->GetWidth() * srcImage->GetHeight(); i++)
    {
        float R = pSrc[offset + 0];
        float G = pSrc[offset + 1];
        float B = pSrc[offset + 2];

        float L = 0.2126f * R + 0.7152f * G + 0.0722f * B;
        float Ld = L / (1.0f + L);
        float scale = (L > 0.0f) ? (Ld / L) : 1.0f;

        R = Clamp(R * scale, 0.0f, 1.0f);
        G = Clamp(G * scale, 0.0f, 1.0f);
        B = Clamp(B * scale, 0.0f, 1.0f);

        pDst[i * 4 + 0] = (uint8_t)(R * 255.0f + 0.5f);
        pDst[i * 4 + 1] = (uint8_t)(G * 255.0f + 0.5f);
        pDst[i * 4 + 2] = (uint8_t)(B * 255.0f + 0.5f);
        pDst[i * 4 + 3] = 255;

        offset += 3;
    }

    return result;
}

int main(int argc, char* argv[])
{
    ToolOptions opts;
    if (!ParseArgs(argc, argv, opts))
    {
        return 1;
    }

    std::cout << "=== HDRToCubemapTool ===" << std::endl;
    std::cout << "Input:  " << opts.inputFile << std::endl;
    std::cout << "Output: " << opts.outputDir << std::endl;

    // Ensure output directory exists
    fs::path outDir(opts.outputDir);
    if (!fs::exists(outDir))
    {
        fs::create_directories(outDir);
    }

    std::string baseName = fs::path(opts.inputFile).stem().string();

    // ========== Step 1: Load HDR image ==========
    std::cout << "\n[1/5] Loading HDR image..." << std::endl;

    imagecodec::VImage image;
    if (!imagecodec::ImageDecoder::DecodeFile(opts.inputFile.c_str(), &image))
    {
        std::cerr << "Error: Failed to decode HDR file: " << opts.inputFile << std::endl;
        return 1;
    }
    std::cout << "  Loaded: " << image.GetWidth() << "x" << image.GetHeight()
              << " format=" << image.GetFormat() << std::endl;

    // ========== Step 2: Convert to vertical cross cubemap ==========
    std::cout << "\n[2/5] Converting equirectangular to cubemap..." << std::endl;

    imagecodec::VImagePtr crossImage = AssetProcess::ConvertEquirectangularMapToVerticalCross(&image);
    if (!crossImage)
    {
        std::cerr << "Error: Failed to convert to cubemap." << std::endl;
        return 1;
    }
    std::cout << "  Cross layout: " << crossImage->GetWidth() << "x" << crossImage->GetHeight() << std::endl;

    // ========== Step 3: Split into 6 faces ==========
    std::cout << "\n[3/5] Splitting into 6 cubemap faces..." << std::endl;

    std::vector<imagecodec::VImagePtr> faces = AssetProcess::ConvertVerticalCrossToCubeMapFaces(crossImage.get());
    if (faces.size() != 6)
    {
        std::cerr << "Error: Failed to split cubemap into 6 faces." << std::endl;
        return 1;
    }
    std::cout << "  Face size: " << faces[0]->GetWidth() << "x" << faces[0]->GetHeight() << std::endl;

    // ========== Step 4: Save cubemap as KTX ==========
    std::cout << "\n[4/5] Saving cubemap KTX (BC6H compressed)..." << std::endl;

    {
        AssetProcess::TextureImporter textureImporter;
        AssetProcess::TextureImportSettings importSettings;
        importSettings.mipmapMode = AssetProcess::MipmapMode::None;

        std::vector<uint8_t> ktxData = textureImporter.GenerateKTXCubemapData(faces, importSettings);
        if (ktxData.empty())
        {
            std::cerr << "Error: Failed to generate KTX cubemap data." << std::endl;
            return 1;
        }

        fs::path cubemapPath = outDir / (baseName + "_cubemap.ktx");
        std::ofstream outFile(cubemapPath.string(), std::ios::binary);
        if (!outFile.is_open())
        {
            std::cerr << "Error: Failed to open output file: " << cubemapPath << std::endl;
            return 1;
        }
        outFile.write(reinterpret_cast<const char*>(ktxData.data()), ktxData.size());
        outFile.close();

        std::cout << "  Saved: " << cubemapPath << " (" << ktxData.size() << " bytes)" << std::endl;
    }

    // Optionally save PNG faces
    if (opts.savePNG)
    {
        const char* faceNames[] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };
        for (size_t i = 0; i < faces.size(); ++i)
        {
            imagecodec::VImagePtr rgbaImage = ConvertHDRToRGBA8(faces[i].get());
            if (!rgbaImage) continue;

            std::ostringstream oss;
            oss << baseName << "_face_" << faceNames[i] << ".png";
            fs::path facePath = outDir / oss.str();

            imagecodec::ImageEncoder::EncodeFile(facePath.string().c_str(), *rgbaImage,
                                                  imagecodec::ImageStoreFormat::kPNG_Format, 100);
            std::cout << "  Saved face: " << facePath << std::endl;
        }
    }

    // ========== Step 5: Generate IBL assets ==========
    if (opts.generateIBL)
    {
        std::cout << "\n[5/5] Generating IBL assets..." << std::endl;

        // 5a. Irradiance map
        {
            fs::path irradiancePath = outDir / (baseName + "_irradiance.ktx");
            AssetProcess::GenerateIrradianceMap_Texture(irradiancePath.string(), faces,
                                                         opts.irradianceSize, opts.samples);
            std::cout << "  Irradiance map saved: " << irradiancePath << std::endl;
        }

        // 5b. Prefiltered environment map
        if (opts.generatePrefilter)
        {
            fs::path prefilterPath = outDir / (baseName + "_prefilter.ktx");
            AssetProcess::GeneratePrefilteredEnvMap_Texture(prefilterPath.string(), faces,
                                                             opts.prefilterSize, opts.samples);
            std::cout << "  Prefiltered env map saved: " << prefilterPath << std::endl;
        }
    }
    else
    {
        std::cout << "\n[5/5] Skipping IBL generation (--no-ibl)" << std::endl;
    }

    std::cout << "\n=== Done! ===" << std::endl;
    return 0;
}
