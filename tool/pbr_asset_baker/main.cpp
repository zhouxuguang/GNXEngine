//
//  pbr_asset_baker.cpp
//  GNXEngine offline PBR asset baking tool
//
//  Host-only CLI that converts source art (jpg/png/hdr/gltf) into engine
//  .texture / .meshasset container files under data_asset/pbr/.
//
//  打包逻辑全部收敛到引擎内部的 AssetProcess::AssetPackager：
//    - 模型 assimp 解析（统一预处理参数）
//    - 贴图 RGB->RGBA + sRGB/线性标签 + BC7 压缩 + mip 链
//    - HDR -> 天空盒/IBL（BC6H）烘焙
//    - 程序化基础几何（sphere/plane）
//    - AssetFileHeader 容器写入（AssetManager::AssetContainerWriter）
//  本工具只负责命令行参数解析，不再手搓任何打包/写盘逻辑。
//
//  Sub-commands:
//    texture <src_img> <out.texture> --type albedo|normal|orm|ao|emissive
//    ibl <src_env.hdr> <out_prefix> [--face-size N] [--samples N]
//    cubemap <src_env.hdr> <out.texture> [--face-size N]
//    mesh <src_model.gltf|glb|obj> <out.meshasset>
//    primitives <sphere|plane> <out.meshasset>
//

#include <iostream>
#include <string>
#include <cstdlib>

#include "Runtime/AssetProcess/include/AssetPackager.h"

using namespace AssetProcess;

namespace
{
void PrintUsage(const char* prog)
{
    std::cout << "Usage:\n"
              << "  " << prog << " texture <src_img> <out.texture> --type albedo|normal|orm|ao|emissive\n"
              << "  " << prog << " ibl <src_env.hdr> <out_prefix> [--face-size N] [--samples N]\n"
              << "  " << prog << " cubemap <src_env.hdr> <out.texture> [--face-size N]\n"
              << "  " << prog << " mesh <src_model> <out.meshasset>\n"
              << "  " << prog << " primitives <sphere|plane> <out.meshasset>\n";
}

TextureMapType ParseTextureType(const std::string& type)
{
    if (type == "normal")   return TextureMapType::Normal;
    if (type == "orm")      return TextureMapType::Orm;
    if (type == "ao")       return TextureMapType::AO;
    if (type == "emissive") return TextureMapType::Emissive;
    return TextureMapType::Albedo;   // 默认 / "albedo"
}
} // namespace

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "texture")
    {
        if (argc < 4)
        {
            PrintUsage(argv[0]);
            return 1;
        }
        std::string src = argv[2];
        std::string out = argv[3];
        std::string type = "albedo";
        for (int i = 4; i < argc; ++i)
        {
            std::string a = argv[i];
            if (a == "--type" && i + 1 < argc)
            {
                type = argv[++i];
            }
        }
        return AssetPackager::Pack2DTextureFromFile(src, out, ParseTextureType(type)) ? 0 : 1;
    }
    else if (cmd == "ibl")
    {
        if (argc < 4)
        {
            PrintUsage(argv[0]);
            return 1;
        }
        std::string src = argv[2];
        std::string prefix = argv[3];
        uint32_t faceSize = 128;
        uint32_t samples = 256;
        for (int i = 4; i < argc; ++i)
        {
            std::string a = argv[i];
            if (a == "--face-size" && i + 1 < argc) faceSize = (uint32_t)atoi(argv[++i]);
            else if (a == "--samples" && i + 1 < argc) samples = (uint32_t)atoi(argv[++i]);
        }
        return AssetPackager::BakeIBLFromHDR(src, prefix, faceSize, samples) ? 0 : 1;
    }
    else if (cmd == "cubemap")
    {
        if (argc < 4)
        {
            PrintUsage(argv[0]);
            return 1;
        }
        std::string src = argv[2];
        std::string out = argv[3];
        uint32_t faceSize = 512;
        for (int i = 4; i < argc; ++i)
        {
            std::string a = argv[i];
            if (a == "--face-size" && i + 1 < argc) faceSize = (uint32_t)atoi(argv[++i]);
        }
        return AssetPackager::PackEnvironmentCubemapFromHDR(src, out, faceSize) ? 0 : 1;
    }
    else if (cmd == "mesh")
    {
        if (argc < 4)
        {
            PrintUsage(argv[0]);
            return 1;
        }
        std::string src = argv[2];
        std::string out = argv[3];
        return AssetPackager::PackMeshFromFile(src, out) ? 0 : 1;
    }
    else if (cmd == "primitives")
    {
        if (argc < 4)
        {
            PrintUsage(argv[0]);
            return 1;
        }
        std::string kind = argv[2];
        std::string out = argv[3];
        return AssetPackager::PackPrimitiveMesh(kind, out) ? 0 : 1;
    }
    else
    {
        PrintUsage(argv[0]);
        return 1;
    }

    return 0;
}
