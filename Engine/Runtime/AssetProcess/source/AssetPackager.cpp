//
//  AssetPackager.cpp
//  GNXEngine
//
//  引擎级离线资产打包实现。逻辑从 tool/pbr_asset_baker/main.cpp 收敛到引擎，
//  工具侧仅保留 CLI 参数解析。
//

#include "AssetPackager.h"
#include "Runtime/AssetProcess/include/AssimpMeshImporter.h"
#include "Runtime/AssetProcess/include/TextureImporter.h"
#include "Runtime/AssetProcess/source/IBLBaker/PBRBase.h"
#include "Runtime/AssetProcess/source/TextureProcess/EnvHdrProcess.h"
#include "Runtime/AssetManager/include/AssetContainerWriter.h"
#include "Runtime/AssetManager/include/MeshMessageUtil.h"
#include "Runtime/AssetManager/include/TextureMessageUtil.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "Runtime/ImageCodec/include/VImage.h"
#include "Runtime/RenderSystem/include/mesh/Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/cimport.h>

#include <filesystem>
#include <vector>
#include <cmath>

namespace fs = std::filesystem;
using namespace imagecodec;

NS_ASSETPROCESS_BEGIN

namespace
{
// 引擎统一的 assimp 模型预处理参数（与 AssimpAssetImporter::ImportFromFile 一致）
constexpr unsigned int kModelImportFlags =
    aiProcess_SplitLargeMeshes |
    aiProcess_JoinIdenticalVertices |
    aiProcess_Triangulate |
    aiProcess_SortByPType |
    aiProcess_GenNormals |
    aiProcess_CalcTangentSpace |
    aiProcess_OptimizeMeshes |
    aiProcess_RemoveRedundantMaterials |
    aiProcess_OptimizeGraph |
    aiProcess_GenBoundingBoxes |
    aiProcess_FixInfacingNormals |
    aiProcess_JoinIdenticalVertices;

bool EncodeMeshMessageToContainer(const RenderSystem::MeshPtr& mesh,
                                  const std::string& outFile,
                                  const std::string& assetName,
                                  AssetManager::AssetType type)
{
    if (!mesh)
    {
        LOG_ERROR("AssetPackager: null mesh");
        return false;
    }
    ByteVectorPtr encoded = AssetManager::MeshMessageUtil::EncodeMeshMessage(mesh.get());
    if (!encoded || encoded->empty())
    {
        LOG_ERROR("AssetPackager: MeshMessage encode failed: %s", outFile.c_str());
        return false;
    }
    std::vector<uint8_t> pbData(encoded->begin(), encoded->end());
    return AssetManager::AssetContainerWriter::WriteAssetFile(outFile, type, assetName, pbData);
}

// ==================== 基础几何生成（sphere/plane） ====================

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

bool BuildSphereMesh(RenderSystem::MeshPtr mesh, float radius, int segments, int rings)
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

bool BuildPlaneMesh(RenderSystem::MeshPtr mesh, float width, float depth)
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

// 从源文件名取 stem 作为默认资产名
std::string GetStemName(const std::string& path)
{
    return fs::path(path).stem().string();
}

bool IsColorMap(TextureMapType type)
{
    return type == TextureMapType::Albedo || type == TextureMapType::Emissive;
}

// 把 KTX 字节编码为 TextureMessage pb，再写成 .texture 容器
// （.texture 载荷必须是 TextureMessage pb；运行时经 DecodeTextureMessage 还原 KTX）
bool WriteTextureContainer(const std::string& outPath,
                           const std::vector<uint8_t>& ktxData,
                           const std::string& assetName,
                           uint32_t flags = AssetManager::AssetFileFlags::NONE)
{
    ByteVectorPtr encoded = AssetManager::TextureMessageUtil::EncodeTextureMessage(ktxData.data(),
                                                                                   (uint32_t)ktxData.size());
    if (!encoded || encoded->empty())
    {
        LOG_ERROR("AssetPackager: TextureMessage encode failed for %s", outPath.c_str());
        return false;
    }
    std::vector<uint8_t> pbData(encoded->begin(), encoded->end());
    return AssetManager::AssetContainerWriter::WriteAssetFile(
        outPath, AssetManager::AssetType::Texture, assetName, pbData, flags);
}

} // namespace

// ==================== 模型打包 ====================

bool AssetPackager::PackMeshFromFile(const std::string& srcModel,
                                     const std::string& outMeshAsset,
                                     const std::string& assetNameIn)
{
    std::string assetName = assetNameIn.empty() ? GetStemName(srcModel) : assetNameIn;

    // 纯 CPU assimp 解析（引擎统一预处理参数）
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(srcModel.c_str(), kModelImportFlags);
    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode || !scene->HasMeshes())
    {
        LOG_ERROR("AssetPackager: assimp failed to load model: %s (%s)", srcModel.c_str(),
                  importer.GetErrorString() ? importer.GetErrorString() : "");
        return false;
    }

    std::vector<uint8_t> pbData;
    AssimpMeshImporter meshImporter(scene, "");
    if (!meshImporter.EncodeMeshToMemory(pbData))
    {
        LOG_ERROR("AssetPackager: failed to encode mesh: %s", srcModel.c_str());
        return false;
    }

    LOG_INFO("AssetPackager: encoded mesh %s: %zu bytes pb, %u vertices",
             srcModel.c_str(), pbData.size(), meshImporter.GetVertexCount());

    return AssetManager::AssetContainerWriter::WriteAssetFile(
        outMeshAsset, AssetManager::AssetType::Mesh, assetName, pbData);
}

bool AssetPackager::PackPrimitiveMesh(const std::string& kind,
                                      const std::string& outMeshAsset)
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
        LOG_ERROR("AssetPackager: unknown primitive '%s' (sphere|plane)", kind.c_str());
        return false;
    }

    return EncodeMeshMessageToContainer(mesh, outMeshAsset, kind, AssetManager::AssetType::Mesh);
}

// ==================== 2D 贴图打包 ====================

bool AssetPackager::Pack2DTextureFromFile(const std::string& srcImage,
                                          const std::string& outTexture,
                                          TextureMapType type,
                                          const std::string& assetNameIn)
{
    std::string assetName = assetNameIn.empty() ? GetStemName(srcImage) : assetNameIn;

    // 1. 解码图片
    VImage image;
    if (!ImageDecoder::DecodeFile(srcImage.c_str(), &image))
    {
        LOG_ERROR("AssetPackager: failed to decode image: %s", srcImage.c_str());
        return false;
    }
    LOG_INFO("AssetPackager: decoded %s: %ux%u format=%d", srcImage.c_str(),
             image.GetWidth(), image.GetHeight(), image.GetFormat());

    // 2. RGB8/SRGB8 -> RGBA8/SRGBA8（BC7 需要 4 通道）
    VImagePtr img = std::make_shared<VImage>();
    ImagePixelFormat srcFormat = image.GetFormat();
    bool colorMap = IsColorMap(type);
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
            dstFormat = FORMAT_SRGB8_ALPHA8;   // 颜色图标记 sRGB
        }
        else
        {
            dstFormat = FORMAT_RGBA8;          // 线性数据
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

        // albedo/emissive 是颜色贴图：字节为 sRGB 编码 → 打 sRGB 标签
        ImagePixelFormat outFormat = srcFormat;
        if (colorMap && srcFormat == FORMAT_RGBA8)
        {
            outFormat = FORMAT_SRGB8_ALPHA8;
        }
        img->SetImageInfo(outFormat, image.GetWidth(), image.GetHeight(), dstData, free);
    }

    // 3. 生成 KTX（BC7 压缩 + mip 链，格式由 VImage 标签决定：
    //    SRGB8_ALPHA8 -> BC7_SRGB, RGBA8 -> BC7_UNORM）
    TextureImporter importer;
    TextureImportSettings settings;
    settings.mipmapMode = MipmapMode::Auto;
    settings.enableCompression = true;
    settings.colorSpace = colorMap ? TextureColorSpace::sRGB : TextureColorSpace::Linear;

    std::vector<uint8_t> ktxData = importer.GenerateKTXData(img, settings);
    if (ktxData.empty())
    {
        LOG_ERROR("AssetPackager: failed to generate KTX for %s", srcImage.c_str());
        return false;
    }

    // 4. 写 .texture 容器（KTX -> TextureMessage pb -> 容器文件）
    return WriteTextureContainer(outTexture, ktxData, assetName,
                                 AssetManager::AssetFileFlags::COMPRESSED);
}

// ==================== 环境/IBL 打包 ====================

namespace
{
// HDR -> 6 面 cubemap（RGB32Float faces）
std::vector<VImagePtr> LoadEnvironmentFaces(const std::string& hdrFile)
{
    VImage image;
    if (!ImageDecoder::DecodeFile(hdrFile.c_str(), &image))
    {
        LOG_ERROR("AssetPackager: failed to decode HDR: %s", hdrFile.c_str());
        return {};
    }
    LOG_INFO("AssetPackager: HDR loaded: %ux%u format=%d", image.GetWidth(), image.GetHeight(), image.GetFormat());

    VImagePtr cross = ConvertEquirectangularMapToVerticalCross(&image);
    if (!cross)
    {
        LOG_ERROR("AssetPackager: equirect -> vertical cross failed");
        return {};
    }

    std::vector<VImagePtr> faces = ConvertVerticalCrossToCubeMapFaces(cross.get());
    if (faces.size() != 6)
    {
        LOG_ERROR("AssetPackager: failed to split cubemap into 6 faces");
        return {};
    }

    LOG_INFO("AssetPackager: environment faces: %ux%u x6", faces[0]->GetWidth(), faces[0]->GetHeight());
    return faces;
}
} // namespace

bool AssetPackager::PackEnvironmentCubemapFromHDR(const std::string& hdrFile,
                                                  const std::string& outTexture,
                                                  uint32_t faceSize,
                                                  const std::string& assetNameIn)
{
    std::string assetName = assetNameIn.empty() ? GetStemName(outTexture) : assetNameIn;

    std::vector<VImagePtr> faces = LoadEnvironmentFaces(hdrFile);
    if (faces.size() != 6)
    {
        return false;
    }
    (void)faceSize;   // 面尺寸由源 HDR 决定（保持与既有资产一致）

    // RGB32Float -> BC6H_UFLOAT 压缩（TextureImporter 按 VImage 格式自动选 BC6H）
    TextureImporter importer;
    TextureImportSettings s;
    s.mipmapMode = MipmapMode::None;
    std::vector<uint8_t> ktx = importer.GenerateKTXCubemapData(faces, s);
    if (ktx.empty())
    {
        LOG_ERROR("AssetPackager: cubemap KTX encode failed for %s", hdrFile.c_str());
        return false;
    }

    return WriteTextureContainer(outTexture, ktx, assetName,
                                 AssetManager::AssetFileFlags::COMPRESSED);
}

bool AssetPackager::BakeIBLFromHDR(const std::string& hdrFile,
                                   const std::string& outPrefix,
                                   uint32_t faceSize,
                                   uint32_t samples)
{
    std::vector<VImagePtr> faces = LoadEnvironmentFaces(hdrFile);
    if (faces.size() != 6)
    {
        return false;
    }

    // --- BRDF LUT（RG16Float，非压缩） ---
    {
        std::string outPath = outPrefix + "_brdfLUT.texture";
        VImagePtr lut = GenerateBRDFLUT(512, 1024);
        if (!lut)
        {
            LOG_ERROR("AssetPackager: BRDF LUT generation failed");
            return false;
        }
        TextureImporter importer;
        TextureImportSettings s;
        s.mipmapMode = MipmapMode::None;
        s.enableCompression = false;
        std::vector<uint8_t> ktx = importer.GenerateKTXData(lut, s);
        if (ktx.empty())
        {
            LOG_ERROR("AssetPackager: BRDF LUT KTX encode failed");
            return false;
        }
        if (!WriteTextureContainer(outPath, ktx, "brdfLUT"))
        {
            return false;
        }
    }

    // --- Irradiance（漫反射辐照度，32x32，BC6H） ---
    {
        std::string outPath = outPrefix + "_irradiance.texture";
        std::vector<VImagePtr> irrFaces = GenerateIrradianceMap(faces, 32, samples);
        if (irrFaces.empty())
        {
            LOG_ERROR("AssetPackager: irradiance generation failed");
            return false;
        }
        TextureImporter importer;
        TextureImportSettings s;
        s.mipmapMode = MipmapMode::None;
        std::vector<uint8_t> ktx = importer.GenerateKTXCubemapData(irrFaces, s);
        if (ktx.empty())
        {
            LOG_ERROR("AssetPackager: irradiance KTX encode failed");
            return false;
        }
        if (!WriteTextureContainer(outPath, ktx, "env_irradiance",
                                   AssetManager::AssetFileFlags::COMPRESSED))
        {
            return false;
        }
    }

    // --- Prefiltered（高光预过滤，BC6H，逐 mip GGX 卷积） ---
    {
        std::string outPath = outPrefix + "_prefilter.texture";
        std::vector<uint8_t> ktx;
        if (!GeneratePrefilteredEnvMapMipChain_Data(faces, faceSize, samples, ktx))
        {
            LOG_ERROR("AssetPackager: prefiltered env mip-chain generation failed");
            return false;
        }
        if (!WriteTextureContainer(outPath, ktx, "env_prefilter",
                                   AssetManager::AssetFileFlags::COMPRESSED))
        {
            return false;
        }
    }

    return true;
}

NS_ASSETPROCESS_END
