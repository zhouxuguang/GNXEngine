//
//  TerrainGenerator.cpp
//  GNXEngine
//
//  CPU-side terrain mesh generation from procedural heightmap.
//  Phase 1 of the terrain system - heightmap driven terrain.
//

#include "terrain/TerrainGenerator.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/ImageCodec/include/VImage.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <cmath>
#include <algorithm>
#include <vector>

NS_RENDERSYSTEM_BEGIN

//=============================================================================
// Procedural height function (multi-octave sine-wave noise)
//=============================================================================

float TerrainGenerator::ComputeHeight(float x, float z)
{
    // Four octaves of sine waves at different frequencies and amplitudes.
    // Output range is approximately [-75, +75].
    float h = 0.0f;
    h += sinf(x * 0.01f + 0.5f) * cosf(z * 0.013f + 1.3f) * 40.0f;
    h += sinf(x * 0.03f + 2.1f) * cosf(z * 0.027f + 0.7f) * 20.0f;
    h += sinf(x * 0.07f + 5.3f) * cosf(z * 0.061f + 3.1f) * 10.0f;
    h += sinf(x * 0.13f + 1.7f) * cosf(z * 0.17f + 4.2f) * 5.0f;
    return h;
}

//=============================================================================
// Generate terrain mesh
//=============================================================================

MeshPtr TerrainGenerator::GenerateMesh(uint32_t resolution, float worldSizeXZ, float heightScale)
{
    uint32_t vertexCount = resolution * resolution;
    uint32_t indexCount  = (resolution - 1) * (resolution - 1) * 6;

    float step     = worldSizeXZ / (float)(resolution - 1);
    float halfSize = worldSizeXZ * 0.5f;

    std::vector<Vector3f>  positions(vertexCount);
    std::vector<Vector3f>  normals(vertexCount);
    std::vector<Vector4f>  tangents(vertexCount, Vector4f(1, 0, 0, 1));
    std::vector<Vector2f>  texcoords(vertexCount);
    std::vector<uint32_t>  indices(indexCount);

    // ---- Generate vertices ----
    for (uint32_t iz = 0; iz < resolution; ++iz)
    {
        for (uint32_t ix = 0; ix < resolution; ++ix)
        {
            uint32_t idx = iz * resolution + ix;
            float x = -halfSize + (float)ix * step;
            float z = -halfSize + (float)iz * step;
            float y = ComputeHeight(x, z) * heightScale;

            positions[idx]  = Vector3f(x, y, z);
            tangents[idx]  = Vector4f(1.0f, 0.0f, 0.0f, 1.0f);
            texcoords[idx] = Vector2f((float)ix / (float)(resolution - 1),
                                      (float)iz / (float)(resolution - 1));
        }
    }

    // ---- Compute normals via central differences ----
    for (uint32_t iz = 0; iz < resolution; ++iz)
    {
        for (uint32_t ix = 0; ix < resolution; ++ix)
        {
            uint32_t idx = iz * resolution + ix;
            float x = -halfSize + (float)ix * step;
            float z = -halfSize + (float)iz * step;

            float hL = ComputeHeight(x - step, z) * heightScale;
            float hR = ComputeHeight(x + step, z) * heightScale;
            float hD = ComputeHeight(x, z - step) * heightScale;
            float hU = ComputeHeight(x, z + step) * heightScale;

            // Two edge vectors in the XZ plane (with Y as height)
            Vector3f e1(2.0f * step, hR - hL, 0.0f);  // left -> right
            Vector3f e2(0.0f, hU - hD, 2.0f * step);  // down  -> up

            // Normal = e2 x e1 (points upward for flat terrain)
            normals[idx] = Vector3f::CrossProduct(e2, e1).Normalize();
        }
    }

    // ---- Generate triangle indices (CCW winding, viewed from +Y) ----
    uint32_t iidx = 0;
    for (uint32_t iz = 0; iz < resolution - 1; ++iz)
    {
        for (uint32_t ix = 0; ix < resolution - 1; ++ix)
        {
            uint32_t v00 = iz * resolution + ix;
            uint32_t v10 = v00 + 1;
            uint32_t v01 = v00 + resolution;
            uint32_t v11 = v01 + 1;

            indices[iidx++] = v00;
            indices[iidx++] = v10;
            indices[iidx++] = v01;

            indices[iidx++] = v10;
            indices[iidx++] = v11;
            indices[iidx++] = v01;
        }
    }

    // ---- Build Mesh object ----
    MeshPtr mesh = std::make_shared<Mesh>();

    VertexData& vertexData = mesh->GetVertexData();
    uint32_t stride = sizeof(Vector3f) + sizeof(Vector4f) + sizeof(Vector3f) + sizeof(Vector2f); // 48 bytes
    vertexData.Resize(vertexCount, stride);

    ChannelInfo* channels = vertexData.GetChannels();
    channels[kShaderChannelPosition].offset  = 0;
    channels[kShaderChannelPosition].format   = VertexFormatFloat3;
    channels[kShaderChannelPosition].stride  = sizeof(Vector3f);

    channels[kShaderChannelTangent].offset   = positions.size() * sizeof(Vector3f);
    channels[kShaderChannelTangent].format   = VertexFormatFloat4;
    channels[kShaderChannelTangent].stride   = 16;

    channels[kShaderChannelNormal].offset     = positions.size() * sizeof(Vector3f)
                                                + tangents.size() * sizeof(Vector4f);
    channels[kShaderChannelNormal].format     = VertexFormatFloat3;
    channels[kShaderChannelNormal].stride     = 12;

    channels[kShaderChannelTexCoord0].offset  = positions.size() * sizeof(Vector3f)
                                                + tangents.size() * sizeof(Vector4f)
                                                + normals.size() * sizeof(Vector3f);
    channels[kShaderChannelTexCoord0].format  = VertexFormatFloat2;
    channels[kShaderChannelTexCoord0].stride  = 8;

    mesh->SetPositions(positions.data(), vertexCount);
    mesh->SetTangents(tangents.data(), vertexCount);
    mesh->SetNormals(normals.data(), vertexCount);
    mesh->SetUv(0, texcoords.data(), vertexCount);
    mesh->SetIndices(indices.data(), indexCount);

    SubMeshInfo subMeshInfo;
    subMeshInfo.firstIndex  = 0;
    subMeshInfo.indexCount  = indexCount;
    subMeshInfo.vertexCount = vertexCount;
    subMeshInfo.topology    = PrimitiveMode_TRIANGLES;
    mesh->AddSubMeshInfo(subMeshInfo);

    mesh->SetUpBuffer();

    LOG_INFO("Terrain mesh generated: %ux%u = %u vertices, %u triangles",
             resolution, resolution, vertexCount, indexCount / 3);

    return mesh;
}

//=============================================================================
// Generate height-based diffuse texture
//=============================================================================

static Vector3f LerpColor(const Vector3f& a, const Vector3f& b, float t)
{
    return Vector3f(a.x + (b.x - a.x) * t,
                    a.y + (b.y - a.y) * t,
                    a.z + (b.z - a.z) * t);
}

RenderCore::RCTexture2DPtr TerrainGenerator::GenerateDiffuseTexture(
    uint32_t resolution, float worldSizeXZ, float heightScale)
{
    using namespace imagecodec;
    using namespace RenderCore;

    auto renderDevice = GetRenderDevice();

    uint32_t texSize = resolution;
    float step     = worldSizeXZ / (float)(resolution - 1);
    float halfSize = worldSizeXZ * 0.5f;

    // Allocate RGBA8 pixel buffer
    uint32_t bytesPerRow = texSize * 4;
    uint32_t dataSize    = bytesPerRow * texSize;
    uint8_t* pixels = (uint8_t*)malloc(dataSize);

    // Height range for normalization (matches ComputeHeight output range)
    const float kMinHeight = -75.0f;
    const float kMaxHeight =  75.0f;

    // Color palette: dark green -> green -> brown -> gray -> white
    const Vector3f colorDeepGreen (0.10f, 0.25f, 0.06f);
    const Vector3f colorGreen     (0.22f, 0.42f, 0.10f);
    const Vector3f colorBrown     (0.45f, 0.35f, 0.20f);
    const Vector3f colorRock      (0.50f, 0.48f, 0.45f);
    const Vector3f colorSnow      (0.92f, 0.93f, 0.95f);

    for (uint32_t iz = 0; iz < texSize; ++iz)
    {
        for (uint32_t ix = 0; ix < texSize; ++ix)
        {
            float x = -halfSize + (float)ix * step;
            float z = -halfSize + (float)iz * step;
            float h = ComputeHeight(x, z) * heightScale;

            // Normalize height to [0, 1]
            float t = (h - kMinHeight) / (kMaxHeight - kMinHeight);
            t = std::clamp(t, 0.0f, 1.0f);

            // Height-based color mapping with smooth transitions
            Vector3f color;
            if (t < 0.25f)
            {
                color = LerpColor(colorDeepGreen, colorGreen, t / 0.25f);
            }
            else if (t < 0.45f)
            {
                color = LerpColor(colorGreen, colorBrown, (t - 0.25f) / 0.20f);
            }
            else if (t < 0.65f)
            {
                color = LerpColor(colorBrown, colorRock, (t - 0.45f) / 0.20f);
            }
            else
            {
                color = LerpColor(colorRock, colorSnow, (t - 0.65f) / 0.35f);
            }

            uint32_t pixIdx = (iz * texSize + ix) * 4;
            pixels[pixIdx + 0] = (uint8_t)(std::clamp(color.x, 0.0f, 1.0f) * 255.0f);
            pixels[pixIdx + 1] = (uint8_t)(std::clamp(color.y, 0.0f, 1.0f) * 255.0f);
            pixels[pixIdx + 2] = (uint8_t)(std::clamp(color.z, 0.0f, 1.0f) * 255.0f);
            pixels[pixIdx + 3] = 255;
        }
    }

    // Create VImage wrapper
    VImagePtr image = std::make_shared<VImage>();
    image->SetImageInfo(FORMAT_RGBA8, texSize, texSize, pixels, free);

    // Diffuse/albedo textures are sRGB-encoded, use SRGB format for correct gamma decoding
    RCTexture2DPtr texture = renderDevice->CreateTexture2D(
        kTexFormatSRGB8_ALPHA8,
        TextureUsage::TextureUsageShaderRead,
        texSize, texSize, 1);

    Rect2D rect(0, 0, texSize, texSize);
    texture->ReplaceRegion(rect, 0, image->GetImageData(), image->GetBytesPerRow());

    LOG_INFO("Terrain diffuse texture generated: %ux%u", texSize, texSize);

    return texture;
}

//=============================================================================
// Heightmap sampling helper
//=============================================================================

static float SampleHeightBilinear(const imagecodec::VImage& img, float u, float v)
{
    float fx = u * (img.GetWidth() - 1);
    float fy = v * (img.GetHeight() - 1);
    uint32_t x0 = (uint32_t)fx, y0 = (uint32_t)fy;
    uint32_t x1 = std::min(x0 + 1, img.GetWidth() - 1);
    uint32_t y1 = std::min(y0 + 1, img.GetHeight() - 1);
    float dx = fx - x0, dy = fy - y0;
    uint8_t* p = img.GetImageData();
    uint32_t rowBytes = img.GetBytesPerRow();

    auto getPixel = [&](uint32_t x, uint32_t y) -> float {
        if (img.GetFormat() == imagecodec::FORMAT_GRAY16) {
            uint16_t val = *(uint16_t*)(p + y * rowBytes + x * 2);
            return val / 65535.0f;
        } else {
            return p[y * rowBytes + x] / 255.0f;
        }
    };

    float h00 = getPixel(x0, y0);
    float h10 = getPixel(x1, y0);
    float h01 = getPixel(x0, y1);
    float h11 = getPixel(x1, y1);
    return h00 * (1 - dx) * (1 - dy) + h10 * dx * (1 - dy) + h01 * (1 - dx) * dy + h11 * dx * dy;
}

//=============================================================================
// Generate terrain mesh from heightmap image
//=============================================================================

MeshPtr TerrainGenerator::GenerateMeshFromHeightMap(const char* heightmapPath,
                                                     uint32_t resolution,
                                                     float worldSizeXZ,
                                                     float heightScale)
{
    using namespace imagecodec;

    // ---- Load heightmap image ----
    VImage heightmapImage;
    if (!ImageDecoder::DecodeFile(heightmapPath, &heightmapImage))
    {
        LOG_ERROR("Failed to load heightmap: %s", heightmapPath);
        return nullptr;
    }

    if (heightmapImage.GetFormat() != FORMAT_GRAY8 && heightmapImage.GetFormat() != FORMAT_GRAY16)
    {
        LOG_ERROR("Heightmap must be grayscale (GRAY8 or GRAY16), got format %d: %s",
                  heightmapImage.GetFormat(), heightmapPath);
        return nullptr;
    }

    LOG_INFO("Heightmap loaded: %s (%ux%u, format=%s)",
             heightmapPath, heightmapImage.GetWidth(), heightmapImage.GetHeight(),
             heightmapImage.GetFormat() == FORMAT_GRAY16 ? "GRAY16" : "GRAY8");

    // ---- Generate mesh ----
    uint32_t vertexCount = resolution * resolution;
    uint32_t indexCount  = (resolution - 1) * (resolution - 1) * 6;

    float step     = worldSizeXZ / (float)(resolution - 1);
    float halfSize = worldSizeXZ * 0.5f;

    std::vector<Vector3f>  positions(vertexCount);
    std::vector<Vector3f>  normals(vertexCount);
    std::vector<Vector4f>  tangents(vertexCount);
    std::vector<Vector2f>  texcoords(vertexCount);
    std::vector<uint32_t>  indices(indexCount);

    // ---- Generate vertices by sampling heightmap ----
    for (uint32_t iz = 0; iz < resolution; ++iz)
    {
        for (uint32_t ix = 0; ix < resolution; ++ix)
        {
            uint32_t idx = iz * resolution + ix;
            float u = (float)ix / (float)(resolution - 1);
            float v = (float)iz / (float)(resolution - 1);
            float x = -halfSize + (float)ix * step;
            float z = -halfSize + (float)iz * step;
            float y = SampleHeightBilinear(heightmapImage, u, v) * heightScale;

            positions[idx]  = Vector3f(x, y, z);
            texcoords[idx]  = Vector2f(u, v);
        }
    }

    // ---- Compute normals via central differences on the heightmap ----
    for (uint32_t iz = 0; iz < resolution; ++iz)
    {
        for (uint32_t ix = 0; ix < resolution; ++ix)
        {
            uint32_t idx = iz * resolution + ix;

            // Sample neighboring heights (clamped UV)
            float uL = std::max(0.0f, (float)(ix > 0 ? ix - 1 : ix) / (float)(resolution - 1));
            float uR = std::min(1.0f, (float)(ix < resolution - 1 ? ix + 1 : ix) / (float)(resolution - 1));
            float vD = std::max(0.0f, (float)(iz > 0 ? iz - 1 : iz) / (float)(resolution - 1));
            float vU = std::min(1.0f, (float)(iz < resolution - 1 ? iz + 1 : iz) / (float)(resolution - 1));

            float hL = SampleHeightBilinear(heightmapImage, uL, (float)iz / (float)(resolution - 1)) * heightScale;
            float hR = SampleHeightBilinear(heightmapImage, uR, (float)iz / (float)(resolution - 1)) * heightScale;
            float hD = SampleHeightBilinear(heightmapImage, (float)ix / (float)(resolution - 1), vD) * heightScale;
            float hU = SampleHeightBilinear(heightmapImage, (float)ix / (float)(resolution - 1), vU) * heightScale;

            float dx = (uR - uL) * worldSizeXZ;
            float dz = (vU - vD) * worldSizeXZ;

            Vector3f e1(dx, hR - hL, 0.0f);
            Vector3f e2(0.0f, hU - hD, dz);
            normals[idx] = Vector3f::CrossProduct(e2, e1).Normalize();
        }
    }

    // ---- Compute tangents from heightmap derivatives ----
    for (uint32_t iz = 0; iz < resolution; ++iz)
    {
        for (uint32_t ix = 0; ix < resolution; ++ix)
        {
            uint32_t idx = iz * resolution + ix;
            float uR = std::min(1.0f, (float)(ix < resolution - 1 ? ix + 1 : ix) / (float)(resolution - 1));
            float uL = std::max(0.0f, (float)(ix > 0 ? ix - 1 : ix) / (float)(resolution - 1));

            float hR = SampleHeightBilinear(heightmapImage, uR, (float)iz / (float)(resolution - 1)) * heightScale;
            float hL = SampleHeightBilinear(heightmapImage, uL, (float)iz / (float)(resolution - 1)) * heightScale;
            float dhdx = (hR - hL) / ((uR - uL) * worldSizeXZ);

            Vector3f tan3 = Vector3f(1.0f, dhdx, 0.0f).Normalize();
            tangents[idx] = Vector4f(tan3.x, tan3.y, tan3.z, 1.0f);
        }
    }

    // ---- Generate triangle indices (CCW winding, viewed from +Y) ----
    uint32_t iidx = 0;
    for (uint32_t iz = 0; iz < resolution - 1; ++iz)
    {
        for (uint32_t ix = 0; ix < resolution - 1; ++ix)
        {
            uint32_t v00 = iz * resolution + ix;
            uint32_t v10 = v00 + 1;
            uint32_t v01 = v00 + resolution;
            uint32_t v11 = v01 + 1;

            indices[iidx++] = v00;
            indices[iidx++] = v10;
            indices[iidx++] = v01;

            indices[iidx++] = v10;
            indices[iidx++] = v11;
            indices[iidx++] = v01;
        }
    }

    // ---- Build Mesh object ----
    MeshPtr mesh = std::make_shared<Mesh>();

    VertexData& vertexData = mesh->GetVertexData();
    uint32_t stride = sizeof(Vector3f) + sizeof(Vector4f) + sizeof(Vector3f) + sizeof(Vector2f); // 48 bytes
    vertexData.Resize(vertexCount, stride);

    ChannelInfo* channels = vertexData.GetChannels();
    channels[kShaderChannelPosition].offset  = 0;
    channels[kShaderChannelPosition].format   = VertexFormatFloat3;
    channels[kShaderChannelPosition].stride  = sizeof(Vector3f);

    channels[kShaderChannelTangent].offset   = positions.size() * sizeof(Vector3f);
    channels[kShaderChannelTangent].format   = VertexFormatFloat4;
    channels[kShaderChannelTangent].stride   = 16;

    channels[kShaderChannelNormal].offset     = positions.size() * sizeof(Vector3f)
                                                + tangents.size() * sizeof(Vector4f);
    channels[kShaderChannelNormal].format     = VertexFormatFloat3;
    channels[kShaderChannelNormal].stride     = 12;

    channels[kShaderChannelTexCoord0].offset  = positions.size() * sizeof(Vector3f)
                                                + tangents.size() * sizeof(Vector4f)
                                                + normals.size() * sizeof(Vector3f);
    channels[kShaderChannelTexCoord0].format  = VertexFormatFloat2;
    channels[kShaderChannelTexCoord0].stride  = 8;

    mesh->SetPositions(positions.data(), vertexCount);
    mesh->SetTangents(tangents.data(), vertexCount);
    mesh->SetNormals(normals.data(), vertexCount);
    mesh->SetUv(0, texcoords.data(), vertexCount);
    mesh->SetIndices(indices.data(), indexCount);

    SubMeshInfo subMeshInfo;
    subMeshInfo.firstIndex  = 0;
    subMeshInfo.indexCount  = indexCount;
    subMeshInfo.vertexCount = vertexCount;
    subMeshInfo.topology    = PrimitiveMode_TRIANGLES;
    mesh->AddSubMeshInfo(subMeshInfo);

    mesh->SetUpBuffer();

    LOG_INFO("Terrain mesh generated from heightmap: %ux%u = %u vertices, %u triangles",
             resolution, resolution, vertexCount, indexCount / 3);

    return mesh;
}

//=============================================================================
// Load diffuse texture from image file
//=============================================================================

RenderCore::RCTexture2DPtr TerrainGenerator::LoadDiffuseTexture(const char* texturePath)
{
    using namespace imagecodec;
    using namespace RenderCore;

    auto renderDevice = GetRenderDevice();

    VImage textureImage;
    if (!ImageDecoder::DecodeFile(texturePath, &textureImage))
    {
        LOG_ERROR("Failed to load terrain diffuse texture: %s", texturePath);
        return nullptr;
    }

    LOG_INFO("Terrain diffuse texture loaded: %s (%ux%u)", texturePath,
             textureImage.GetWidth(), textureImage.GetHeight());

    // Convert to RGBA8 if needed (the GPU texture format is RGBA8)
    uint32_t texWidth  = textureImage.GetWidth();
    uint32_t texHeight = textureImage.GetHeight();

    // Diffuse/albedo textures are sRGB-encoded, use SRGB format so GPU
    // automatically decodes gamma to linear when sampling in the shader.
    // Without this, PBR lighting will compute with gamma-encoded values and appear too dark.
    RCTexture2DPtr texture = renderDevice->CreateTexture2D(
        kTexFormatSRGB8_ALPHA8,
        TextureUsage::TextureUsageShaderRead,
        texWidth, texHeight, 1);

    if (textureImage.GetFormat() == FORMAT_RGBA8 || textureImage.GetFormat() == FORMAT_SRGB8_ALPHA8)
    {
        // Direct upload
        Rect2D rect(0, 0, texWidth, texHeight);
        texture->ReplaceRegion(rect, 0, textureImage.GetImageData(), textureImage.GetBytesPerRow());
    }
    else if (textureImage.GetFormat() == FORMAT_RGB8 || textureImage.GetFormat() == FORMAT_SRGB8)
    {
        // Convert RGB8 to RGBA8
        uint32_t dstBytesPerRow = texWidth * 4;
        uint8_t* dstData = (uint8_t*)malloc(dstBytesPerRow * texHeight);
        uint8_t* srcData = textureImage.GetImageData();
        uint32_t srcRowBytes = textureImage.GetBytesPerRow();

        for (uint32_t y = 0; y < texHeight; ++y)
        {
            for (uint32_t x = 0; x < texWidth; ++x)
            {
                uint32_t srcIdx = y * srcRowBytes + x * 3;
                uint32_t dstIdx = (y * texWidth + x) * 4;
                dstData[dstIdx + 0] = srcData[srcIdx + 0];
                dstData[dstIdx + 1] = srcData[srcIdx + 1];
                dstData[dstIdx + 2] = srcData[srcIdx + 2];
                dstData[dstIdx + 3] = 255;
            }
        }

        Rect2D rect(0, 0, texWidth, texHeight);
        texture->ReplaceRegion(rect, 0, dstData, dstBytesPerRow);
        free(dstData);
    }
    else
    {
        LOG_ERROR("Unsupported terrain texture format: %d", textureImage.GetFormat());
        return nullptr;
    }

    return texture;
}

NS_RENDERSYSTEM_END
