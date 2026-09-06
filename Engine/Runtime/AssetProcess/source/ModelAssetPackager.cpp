//
//  ModelAssetPackager.cpp
//  GNXEngine
//
//  引擎级【模型级】离线资产打包实现。
//  模型/材质贴图/基础几何 打包逻辑（原 AssetPackager 中模型部分）。
//

#include "ModelAssetPackager.h"
#include "AssetPackHelpers.h"
#include "Runtime/AssetProcess/include/AssimpMeshImporter.h"
#include "Runtime/AssetProcess/include/TextureImporter.h"
#include "Runtime/AssetManager/include/AssetContainerWriter.h"
#include "Runtime/AssetManager/include/MeshMessageUtil.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "Runtime/ImageCodec/include/VImage.h"
#include "Runtime/RenderSystem/include/mesh/Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/cimport.h>

#include <vector>
#include <cmath>

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
        LOG_ERROR("ModelAssetPackager: null mesh");
        return false;
    }
    ByteVectorPtr encoded = AssetManager::MeshMessageUtil::EncodeMeshMessage(mesh.get());
    if (!encoded || encoded->empty())
    {
        LOG_ERROR("ModelAssetPackager: MeshMessage encode failed: %s", outFile.c_str());
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

bool IsColorMap(TextureMapType type)
{
    return type == TextureMapType::Albedo || type == TextureMapType::Emissive;
}

} // namespace

// ==================== 模型打包 ====================

bool ModelAssetPackager::PackMeshFromFile(const std::string& srcModel,
                                          const std::string& outMeshAsset,
                                          const std::string& assetNameIn)
{
    std::string assetName = assetNameIn.empty() ? AssetPackHelpers::GetStemName(srcModel) : assetNameIn;

    // 纯 CPU assimp 解析（引擎统一预处理参数）
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(srcModel.c_str(), kModelImportFlags);
    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode || !scene->HasMeshes())
    {
        LOG_ERROR("ModelAssetPackager: assimp failed to load model: %s (%s)", srcModel.c_str(),
                  importer.GetErrorString() ? importer.GetErrorString() : "");
        return false;
    }

    std::vector<uint8_t> pbData;
    AssimpMeshImporter meshImporter(scene, "");
    if (!meshImporter.EncodeMeshToMemory(pbData))
    {
        LOG_ERROR("ModelAssetPackager: failed to encode mesh: %s", srcModel.c_str());
        return false;
    }

    LOG_INFO("ModelAssetPackager: encoded mesh %s: %zu bytes pb, %u vertices",
             srcModel.c_str(), pbData.size(), meshImporter.GetVertexCount());

    return AssetManager::AssetContainerWriter::WriteAssetFile(
        outMeshAsset, AssetManager::AssetType::Mesh, assetName, pbData);
}

bool ModelAssetPackager::PackPrimitiveMesh(const std::string& kind,
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
        LOG_ERROR("ModelAssetPackager: unknown primitive '%s' (sphere|plane)", kind.c_str());
        return false;
    }

    return EncodeMeshMessageToContainer(mesh, outMeshAsset, kind, AssetManager::AssetType::Mesh);
}

// ==================== 2D 贴图打包 ====================

bool ModelAssetPackager::Pack2DTextureFromFile(const std::string& srcImage,
                                               const std::string& outTexture,
                                               TextureMapType type,
                                               const std::string& assetNameIn)
{
    std::string assetName = assetNameIn.empty() ? AssetPackHelpers::GetStemName(srcImage) : assetNameIn;

    // 1. 解码图片
    VImage image;
    if (!ImageDecoder::DecodeFile(srcImage.c_str(), &image))
    {
        LOG_ERROR("ModelAssetPackager: failed to decode image: %s", srcImage.c_str());
        return false;
    }
    LOG_INFO("ModelAssetPackager: decoded %s: %ux%u format=%d", srcImage.c_str(),
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
        LOG_ERROR("ModelAssetPackager: failed to generate KTX for %s", srcImage.c_str());
        return false;
    }

    // 4. 写 .texture 容器（KTX -> TextureMessage pb -> 容器文件）
    return AssetPackHelpers::WriteTextureContainer(outTexture, ktxData, assetName,
                                                   AssetManager::AssetFileFlags::COMPRESSED);
}

NS_ASSETPROCESS_END
