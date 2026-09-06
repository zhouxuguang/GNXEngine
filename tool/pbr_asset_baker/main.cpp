//
//  pbr_asset_baker.cpp
//  GNXEngine offline PBR asset baking tool
//
//  Host-only CLI that converts source art (jpg/png/hdr/gltf) into engine
//  .texture / .gnxasset container files under data_asset/pbr/.
//
//  Sub-commands:
//    texture <src_img> <out.texture> --type albedo|normal|orm|ao|emissive
//        Decode image -> generate KTX (mipmaps, optional BC compression) ->
//        wrap in TextureMessage pb + AssetFileHeader -> write .texture
//
//    ibl <src_env.hdr> <out_dir> [--face-size N] [--samples N]
//        Convert HDR env -> 6-face cubemap -> generate irradiance + prefiltered
//        mip-chain + BRDF LUT, each wrapped into .texture container files.
//
//    cubemap <src_env.hdr> <out_cube.texture> [--face-size N]
//        HDR equirect -> 6-face cubemap KTX -> .texture (for skybox).
//
//    mesh <src_model.gltf|glb|obj> <out.meshasset>
//        Parse geometry via assimp -> Mesh -> MeshMessage pb -> .gnxasset (Mesh)
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstring>
#include <cstdint>

#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "Runtime/ImageCodec/include/ImageEncoder.h"
#include "Runtime/ImageCodec/include/VImage.h"
#include "Runtime/MathUtil/include/MathUtil.h"
#include "Runtime/AssetProcess/include/TextureImporter.h"
#include "Runtime/AssetProcess/source/IBLBaker/PBRBase.h"
#include "Runtime/AssetProcess/source/TextureProcess/EnvHdrProcess.h"
#include "Runtime/AssetManager/include/AssetFileHeader.h"
#include "Runtime/AssetManager/include/TextureMessageUtil.h"
#include "Runtime/AssetManager/include/MeshMessageUtil.h"
#include "Runtime/AssetProcess/include/AssimpMeshImporter.h"
#include "Runtime/RenderSystem/include/mesh/Mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace fs = std::filesystem;
using namespace imagecodec;

// ============================================================================
// 小工具：把任意 pb bytes 包装成带指定 AssetType 的 .gnxasset 容器
// ============================================================================
static bool WriteAssetContainer(const std::string& outPath,
                                const std::vector<uint8_t>& pbData,
                                const std::string& assetName,
                                AssetManager::AssetType type)
{
    using namespace AssetManager;

    if (pbData.empty())
    {
        LOG_ERROR("WriteAssetContainer: empty pb data for %s", outPath.c_str());
        return false;
    }

    uint64_t hash = AssetFileHeaderUtil::ComputeHash(pbData.data(), pbData.size());
    AssetFileHeader header = AssetFileHeaderUtil::CreateHeader(type, assetName, hash, pbData.size(), AssetFileFlags::NONE);

    fs::path filePath(outPath);
    fs::path parent = filePath.parent_path();
    if (!parent.empty() && !fs::exists(parent))
    {
        fs::create_directories(parent);
    }

    std::ofstream outFile(outPath, std::ios::binary);
    if (!outFile.is_open())
    {
        LOG_ERROR("Failed to open output: %s", outPath.c_str());
        return false;
    }
    if (!AssetFileHeaderUtil::WriteHeader(outFile, header))
    {
        LOG_ERROR("Failed to write header for %s", outPath.c_str());
        return false;
    }
    outFile.write(reinterpret_cast<const char*>(pbData.data()), (std::streamsize)pbData.size());
    outFile.close();

    LOG_INFO("Wrote .gnxasset: %s (%zu bytes pb, type=%u)", outPath.c_str(), pbData.size(), (uint32_t)type);
    return true;
}

// ============================================================================
// 程序化基础几何 meshasset（sphere / plane）
// ============================================================================
namespace
{
void BuildMeshChannels(RenderSystem::Mesh* mesh, uint32_t vertexCount, uint32_t vertexSize,
                       bool hasNormal, bool hasTangent, bool hasUV)
{
    RenderSystem::VertexData& vertexData = mesh->GetVertexData();
    vertexData.Resize(vertexCount, vertexSize);

    RenderSystem::ChannelInfo* channels = vertexData.GetChannels();
    uint32_t offset = 0;
    channels[RenderSystem::kShaderChannelPosition].offset = offset;
    channels[RenderSystem::kShaderChannelPosition].format = VertexFormatFloat3;
    channels[RenderSystem::kShaderChannelPosition].stride = sizeof(mathutil::Vector3f);
    offset += vertexCount * sizeof(mathutil::Vector3f);

    if (hasNormal)
    {
        channels[RenderSystem::kShaderChannelNormal].offset = offset;
        channels[RenderSystem::kShaderChannelNormal].format = VertexFormatFloat3;
        channels[RenderSystem::kShaderChannelNormal].stride = sizeof(mathutil::Vector3f);
        offset += vertexCount * sizeof(mathutil::Vector3f);
    }
    if (hasTangent)
    {
        channels[RenderSystem::kShaderChannelTangent].offset = offset;
        channels[RenderSystem::kShaderChannelTangent].format = VertexFormatFloat4;
        channels[RenderSystem::kShaderChannelTangent].stride = sizeof(mathutil::Vector4f);
        offset += vertexCount * sizeof(mathutil::Vector4f);
    }
    if (hasUV)
    {
        channels[RenderSystem::kShaderChannelTexCoord0].offset = offset;
        channels[RenderSystem::kShaderChannelTexCoord0].format = VertexFormatFloat2;
        channels[RenderSystem::kShaderChannelTexCoord0].stride = sizeof(mathutil::Vector2f);
        offset += vertexCount * sizeof(mathutil::Vector2f);
    }
}
} // namespace

static bool BuildSphereMesh(RenderSystem::MeshPtr mesh, float radius, int segments, int rings)
{
    using namespace mathutil;
    const float PI = 3.14159265358979f;

    int vertexCount = (rings + 1) * (segments + 1);
    int indexCount = rings * segments * 6;

    std::vector<Vector3f> positions(vertexCount);
    std::vector<Vector3f> normals(vertexCount);
    std::vector<Vector4f> tangents(vertexCount, Vector4f(1, 0, 0, 1));
    std::vector<Vector2f> texcoords(vertexCount);
    std::vector<uint32_t> indices(indexCount);

    int idx = 0;
    for (int ring = 0; ring <= rings; ++ring)
    {
        float phi = (float)ring / rings * PI;
        for (int seg = 0; seg <= segments; ++seg)
        {
            float theta = (float)seg / segments * 2.0f * PI;
            float sinPhi = sinf(phi), cosPhi = cosf(phi);
            float sinTheta = sinf(theta), cosTheta = cosf(theta);
            float x = sinPhi * cosTheta;
            float y = cosPhi;
            float z = sinPhi * sinTheta;
            positions[idx] = Vector3f(x * radius, y * radius, z * radius);
            normals[idx] = Vector3f(x, y, z);
            texcoords[idx] = Vector2f((float)seg / segments, (float)ring / rings);
            Vector3f tangent(-sinTheta, 0.0f, cosTheta);
            tangent.Normalize();
            tangents[idx] = Vector4f(tangent.x, tangent.y, tangent.z, 1.0f);
            ++idx;
        }
    }

    idx = 0;
    for (int ring = 0; ring < rings; ++ring)
    {
        int ringStart = ring * (segments + 1);
        int nextRingStart = (ring + 1) * (segments + 1);
        for (int seg = 0; seg < segments; ++seg)
        {
            indices[idx++] = ringStart + seg;
            indices[idx++] = nextRingStart + seg;
            indices[idx++] = nextRingStart + seg + 1;
            indices[idx++] = ringStart + seg;
            indices[idx++] = nextRingStart + seg + 1;
            indices[idx++] = ringStart + seg + 1;
        }
    }

    uint32_t stride = sizeof(Vector3f) + sizeof(Vector4f) + sizeof(Vector3f) + sizeof(Vector2f);
    BuildMeshChannels(mesh.get(), vertexCount, stride, true, true, true);

    mesh->SetPositions(positions.data(), vertexCount);
    mesh->SetTangents(tangents.data(), vertexCount);
    mesh->SetNormals(normals.data(), vertexCount);
    mesh->SetUv(0, texcoords.data(), vertexCount);
    mesh->SetIndices(indices.data(), indexCount);

    RenderSystem::SubMeshInfo sub;
    sub.firstIndex = 0;
    sub.indexCount = indexCount;
    sub.vertexCount = vertexCount;
    sub.topology = PrimitiveMode_TRIANGLES;
    mesh->AddSubMeshInfo(sub);
    return true;
}

static bool BuildPlaneMesh(RenderSystem::MeshPtr mesh, float width, float depth)
{
    using namespace mathutil;
    const int nPoints = 4;
    std::vector<Vector3f> positions = {
        Vector3f(-width * 0.5f, 0, depth * 0.5f),
        Vector3f( width * 0.5f, 0, depth * 0.5f),
        Vector3f( width * 0.5f, 0, -depth * 0.5f),
        Vector3f(-width * 0.5f, 0, -depth * 0.5f)
    };
    std::vector<Vector3f> normals(4, Vector3f(0, 1, 0));
    std::vector<Vector4f> tangents(4, Vector4f(1, 0, 0, 1));
    std::vector<Vector2f> texcoords = { Vector2f(0, 0), Vector2f(1, 0), Vector2f(1, 1), Vector2f(0, 1) };
    std::vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3 };

    uint32_t stride = sizeof(Vector3f) + sizeof(Vector4f) + sizeof(Vector3f) + sizeof(Vector2f);
    BuildMeshChannels(mesh.get(), nPoints, stride, true, true, true);

    mesh->SetPositions(positions.data(), nPoints);
    mesh->SetNormals(normals.data(), nPoints);
    mesh->SetTangents(tangents.data(), nPoints);
    mesh->SetUv(0, texcoords.data(), nPoints);
    mesh->SetIndices(indices.data(), indices.size());

    RenderSystem::SubMeshInfo sub;
    sub.firstIndex = 0;
    sub.indexCount = indices.size();
    sub.vertexCount = nPoints;
    sub.topology = PrimitiveMode_TRIANGLES;
    mesh->AddSubMeshInfo(sub);
    return true;
}

static bool PackPrimitiveMesh(const std::string& kind, const std::string& outFile)
{
    RenderSystem::MeshPtr mesh = std::make_shared<RenderSystem::Mesh>();
    if (kind == "sphere")
    {
        BuildSphereMesh(mesh, 1.0f, 48, 32);
    }
    else if (kind == "plane")
    {
        BuildPlaneMesh(mesh, 20.0f, 20.0f);
    }
    else
    {
        LOG_ERROR("Unknown primitive: %s (sphere|plane)", kind.c_str());
        return false;
    }

    using namespace AssetManager;
    ByteVectorPtr encoded = MeshMessageUtil::EncodeMeshMessage(mesh.get());
    if (!encoded || encoded->empty())
    {
        LOG_ERROR("Failed to encode primitive mesh: %s", kind.c_str());
        return false;
    }
    std::vector<uint8_t> pbData(encoded->begin(), encoded->end());
    return WriteAssetContainer(outFile, pbData, kind, AssetType::Mesh);
}

// ============================================================================
// Mesh 打包：模型文件 -> MeshMessage pb -> .meshasset (AssetType::Mesh)
// ============================================================================
static bool PackMesh(const std::string& srcFile, const std::string& outFile)
{
    using namespace AssetProcess;

    // 纯 CPU assimp 解析（与运行时 MeshAssimpImpoter 相同的预处理参数）
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(srcFile.c_str(),
        aiProcess_SplitLargeMeshes |
        aiProcess_Triangulate |
        aiProcess_SortByPType |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_OptimizeMeshes |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_OptimizeGraph |
        aiProcess_GenBoundingBoxes |
        aiProcess_FixInfacingNormals |
        aiProcess_JoinIdenticalVertices);

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode || !scene->HasMeshes())
    {
        LOG_ERROR("Failed to load model (assimp): %s", srcFile.c_str());
        return false;
    }

    std::vector<uint8_t> pbData;
    AssimpMeshImporter meshImporter(scene, "");
    if (!meshImporter.EncodeMeshToMemory(pbData))
    {
        LOG_ERROR("Failed to encode mesh: %s", srcFile.c_str());
        return false;
    }

    LOG_INFO("Encoded mesh %s: %zu bytes pb, %u vertices",
             srcFile.c_str(), pbData.size(), meshImporter.GetVertexCount());

    std::string assetName = fs::path(srcFile).stem().string();
    return WriteAssetContainer(outFile, pbData, assetName, AssetManager::AssetType::Mesh);
}

// ============================================================================
// 小工具：把 KTX 字节包装成 .texture 容器（AssetFileHeader + TextureMessage pb）
// ============================================================================
static bool WriteTextureContainer(const std::string& outPath,
                                  const std::vector<uint8_t>& ktxData,
                                  const std::string& assetName)
{
    using namespace AssetManager;

    ByteVectorPtr encoded = TextureMessageUtil::EncodeTextureMessage(ktxData.data(), (uint32_t)ktxData.size());
    if (!encoded || encoded->empty())
    {
        LOG_ERROR("Failed to encode TextureMessage for %s", outPath.c_str());
        return false;
    }

    std::vector<uint8_t> pbData(encoded->begin(), encoded->end());
    return WriteAssetContainer(outPath, pbData, assetName, AssetType::Texture);
}

// ============================================================================
// 纹理打包：图片 -> KTX -> .texture
// ============================================================================
static bool Pack2DTexture(const std::string& srcFile, const std::string& outFile, const std::string& type)
{
    using namespace AssetProcess;

    // 1. 解码图片
    VImage image;
    if (!ImageDecoder::DecodeFile(srcFile.c_str(), &image))
    {
        LOG_ERROR("Failed to decode image: %s", srcFile.c_str());
        return false;
    }

    LOG_INFO("Decoded %s: %ux%u format=%d", srcFile.c_str(), image.GetWidth(), image.GetHeight(), image.GetFormat());

    // RGB8/SRGB8 -> RGBA8/SRGBA8（Vulkan/Metal 对 3 通道支持有限，且 BC7 需要 4 通道）
    VImagePtr img = std::make_shared<VImage>();
    ImagePixelFormat srcFormat = image.GetFormat();
    bool colorMap = (type == "albedo" || type == "emissive");
    bool needConvert = (srcFormat == FORMAT_RGB8 || srcFormat == FORMAT_SRGB8);
    if (needConvert)
    {
        uint32_t width = image.GetWidth();
        uint32_t height = image.GetHeight();
        uint8_t* dstData = (uint8_t*)malloc(width * height * 4);
        const uint8_t* srcData = image.GetImageData();
        for (uint32_t i = 0; i < width * height; ++i)
        {
            dstData[i * 4 + 0] = srcData[i * 3 + 0];
            dstData[i * 4 + 1] = srcData[i * 3 + 1];
            dstData[i * 4 + 2] = srcData[i * 3 + 2];
            dstData[i * 4 + 3] = 255;
        }
        ImagePixelFormat dstFormat;
        if (srcFormat == FORMAT_SRGB8 || (srcFormat == FORMAT_RGB8 && colorMap))
        {
            dstFormat = FORMAT_SRGB8_ALPHA8;   // 颜色图标记 sRGB（RGB8 的字节本就是 sRGB）
        }
        else
        {
            dstFormat = FORMAT_RGBA8;          // 线性数据（normal/orm/ao）
        }
        img->SetImageInfo(dstFormat, width, height, dstData, free);
    }
    else
    {
        // 复制数据（避免源 image 析构后悬垂）
        uint32_t bytesPerPix = image.GetBytesPerPixels();
        uint32_t dataSize = image.GetWidth() * image.GetHeight() * bytesPerPix;
        uint8_t* dstData = (uint8_t*)malloc(dataSize);
        memcpy(dstData, image.GetImageData(), dataSize);

        // albedo / emissive 是颜色贴图：字节为 sRGB 编码（jpg/png 解码即 sRGB），
        // 打上 sRGB 格式标签 → KTX 以 BC7_SRGB 存储，GPU 硬件采样时自动 sRGB->线性，
        // 保证 DeferredLighting 拿到线性 albedo。
        ImagePixelFormat outFormat = srcFormat;
        if (colorMap && srcFormat == FORMAT_RGBA8)
        {
            outFormat = FORMAT_SRGB8_ALPHA8;
        }
        img->SetImageInfo(outFormat, image.GetWidth(), image.GetHeight(), dstData, free);
    }

    // 2. 颜色空间：albedo/emissive 是 sRGB 颜色贴图（字节为 sRGB 编码），
    //    normal/orm/ao 为线性数据（colorMap 已在上方声明）。
    bool srgb = colorMap;

    // 3. 生成 KTX（BC7 压缩 + mip 链）。
    //    TextureImporter::GenerateKTXData 会按 VImage 格式自动选压缩格式：
    //      FORMAT_SRGB8_ALPHA8 -> BC7_SRGB（GPU 硬件采样自动 sRGB->线性）
    //      FORMAT_RGBA8        -> BC7_UNORM（normal/orm/ao 线性数据）
    //    并逐 mip 用 stbir 降采样 + BC7 压缩，输出完整 mip 链。
    TextureImporter importer;
    TextureImportSettings settings;
    settings.mipmapMode = MipmapMode::Auto;
    settings.enableCompression = true;
    settings.colorSpace = srgb ? TextureColorSpace::sRGB : TextureColorSpace::Linear;

    std::vector<uint8_t> ktxData = importer.GenerateKTXData(img, settings);
    if (ktxData.empty())
    {
        LOG_ERROR("Failed to generate BC7 KTX for %s", srcFile.c_str());
        return false;
    }

    // 4. 写 .texture 容器
    std::string assetName = fs::path(srcFile).stem().string();
    return WriteTextureContainer(outFile, ktxData, assetName);
}

// ============================================================================
// HDR -> 6 面 cubemap（RGB32Float faces）
// ============================================================================
static std::vector<VImagePtr> LoadEnvironmentFaces(const std::string& hdrFile, uint32_t faceSize)
{
    using namespace AssetProcess;

    VImage image;
    if (!ImageDecoder::DecodeFile(hdrFile.c_str(), &image))
    {
        LOG_ERROR("Failed to decode HDR: %s", hdrFile.c_str());
        return {};
    }
    LOG_INFO("HDR loaded: %ux%u format=%d", image.GetWidth(), image.GetHeight(), image.GetFormat());

    VImagePtr cross = ConvertEquirectangularMapToVerticalCross(&image);
    if (!cross)
    {
        LOG_ERROR("Failed to convert equirect to vertical cross");
        return {};
    }

    std::vector<VImagePtr> faces = ConvertVerticalCrossToCubeMapFaces(cross.get());
    if (faces.size() != 6)
    {
        LOG_ERROR("Failed to split cubemap into 6 faces");
        return {};
    }

    // ConvertVerticalCrossToCubeMapFaces 返回的尺寸由 cross 图决定（通常已是目标 size 的 6 个面）。
    // 若需要统一 faceSize，用 stb 缩放。这里尽量复用。
    LOG_INFO("Environment faces: %ux%u x6", faces[0]->GetWidth(), faces[0]->GetHeight());
    return faces;
}

// ============================================================================
// IBL 打包入口
//   outPrefix: 例如 data_asset/pbr/env -> 生成 env_irradiance.texture 等
// ============================================================================
static bool BakeIBL(const std::string& hdrFile, const std::string& outPrefix,
                    uint32_t faceSize, uint32_t samples)
{
    using namespace AssetProcess;

    std::vector<VImagePtr> faces = LoadEnvironmentFaces(hdrFile, faceSize);
    if (faces.size() != 6)
    {
        return false;
    }

    // --- BRDF LUT ---
    {
        std::string outPath = outPrefix + "_brdfLUT.texture";
        VImagePtr lut = GenerateBRDFLUT(512, 1024);   // RG16Float
        if (!lut)
        {
            LOG_ERROR("BRDF LUT generation failed");
            return false;
        }
        TextureImporter importer;
        TextureImportSettings s;
        s.mipmapMode = MipmapMode::None;
        s.enableCompression = false;
        std::vector<uint8_t> ktx = importer.GenerateKTXData(lut, s);
        WriteTextureContainer(outPath, ktx, "brdfLUT");
    }

    // --- Irradiance（漫反射辐照度，32x32 足够） ---
    {
        std::string outPath = outPrefix + "_irradiance.texture";
        std::vector<VImagePtr> irrFaces = GenerateIrradianceMap(faces, 32, samples);
        if (irrFaces.empty())
        {
            LOG_ERROR("Irradiance generation failed");
            return false;
        }
        // RGB32Float -> BC6H_UFLOAT 压缩（TextureImporter 按 VImage 格式自动选 BC6H）
        TextureImporter importer;
        TextureImportSettings s;
        s.mipmapMode = MipmapMode::None;
        std::vector<uint8_t> ktx = importer.GenerateKTXCubemapData(irrFaces, s);
        if (ktx.empty())
        {
            LOG_ERROR("Irradiance KTX encode failed");
            return false;
        }
        WriteTextureContainer(outPath, ktx, "env_irradiance");
    }

    // --- Prefiltered（高光预过滤，每个 mip 对应 roughness，GGX 卷积） ---
    {
        std::string outPath = outPrefix + "_prefilter.texture";
        std::vector<uint8_t> ktx;
        if (!GeneratePrefilteredEnvMapMipChain_Data(faces, faceSize, samples, ktx))
        {
            LOG_ERROR("Prefiltered env mip-chain generation failed");
            return false;
        }
        WriteTextureContainer(outPath, ktx, "env_prefilter");
    }

    return true;
}

// ============================================================================
// Cubemap（环境天空盒）打包
// ============================================================================
static bool PackEnvironmentCubemap(const std::string& hdrFile, const std::string& outFile,
                                   uint32_t faceSize)
{
    using namespace AssetProcess;

    std::vector<VImagePtr> faces = LoadEnvironmentFaces(hdrFile, faceSize);
    if (faces.size() != 6)
    {
        return false;
    }

    // RGB32Float -> BC6H_UFLOAT 压缩（TextureImporter 按 VImage 格式自动选 BC6H）
    TextureImporter importer;
    TextureImportSettings s;
    s.mipmapMode = MipmapMode::None;
    std::vector<uint8_t> ktx = importer.GenerateKTXCubemapData(faces, s);
    if (ktx.empty())
    {
        LOG_ERROR("Failed to generate skybox cubemap KTX");
        return false;
    }
    return WriteTextureContainer(outFile, ktx, "env_cubemap");
}

// ============================================================================
// 用法
// ============================================================================
static void PrintUsage(const char* prog)
{
    std::cout << "Usage:\n"
              << "  " << prog << " texture <src_img> <out.texture> --type albedo|normal|orm|ao|emissive\n"
              << "  " << prog << " ibl <src_env.hdr> <out_prefix> [--face-size N] [--samples N]\n"
              << "  " << prog << " cubemap <src_env.hdr> <out.texture> [--face-size N]\n"
              << "  " << prog << " mesh <src_model> <out.meshasset>\n"
              << "  " << prog << " primitives <sphere|plane> <out.meshasset>\n";
}

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
        return Pack2DTexture(src, out, type) ? 0 : 1;
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
        return BakeIBL(src, prefix, faceSize, samples) ? 0 : 1;
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
        return PackEnvironmentCubemap(src, out, faceSize) ? 0 : 1;
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
        return PackMesh(src, out) ? 0 : 1;
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
        return PackPrimitiveMesh(kind, out) ? 0 : 1;
    }
    else
    {
        PrintUsage(argv[0]);
        return 1;
    }

    return 0;
}
