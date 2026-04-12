//
//  PBRFrameWork.cpp
//  pbr
//
//  PBR Demo - Demonstrates Physically Based Rendering capabilities:
//  1. DamagedHelmet glTF model with full PBR texture set
//  2. Material sphere grid showing metallic/roughness variations  
//  3. Textured ground plane with correct PBR material response
//

#include "PBRFrameWork.h"
#include "Runtime/RenderSystem/include/mesh/MeshRenderer.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/RenderSystem/include/ImageTextureUtil.h"
#include "Runtime/RenderSystem/include/Transform.h"
#include "Runtime/RenderSystem/include/TextureSlot.h"
#include "Runtime/MathUtil/include/MathUtil.h"
#include "Runtime/ImageCodec/include/VImage.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "Runtime/RenderSystem/include/SkyBox.h"
#include "Runtime/RenderSystem/include/SkyBoxNode.h"
#include <algorithm>
#include <cmath>

using namespace mathutil;

// ============================================================
// Helper: Create a UV sphere mesh for PBR material preview spheres
// ============================================================
static RenderSystem::MeshPtr CreateSphereMesh(float radius, int segments, int rings)
{
    const float PI = 3.14159265358979f;
    const float TWO_PI = 2.0f * PI;
    
    int vertexCount = (rings + 1) * (segments + 1);
    int indexCount = rings * segments * 6;
    
    std::vector<Vector3f> positions(vertexCount);
    std::vector<Vector3f> normals(vertexCount);
    std::vector<Vector4f> tangents(vertexCount, Vector4f(1, 0, 0, 1));
    std::vector<Vector2f> texcoords(vertexCount);
    std::vector<uint32_t> indices(indexCount);
    
    // Generate vertices
    int idx = 0;
    for (int ring = 0; ring <= rings; ++ring)
    {
        float phi = (float)ring / rings * PI;
        for (int seg = 0; seg <= segments; ++seg)
        {
            float theta = (float)seg / segments * TWO_PI;
            
            float sinPhi = sinf(phi);
            float cosPhi = cosf(phi);
            float sinTheta = sinf(theta);
            float cosTheta = cosf(theta);
            
            float x = sinPhi * cosTheta;
            float y = cosPhi;
            float z = sinPhi * sinTheta;
            
            positions[idx] = Vector3f(x * radius, y * radius, z * radius);
            normals[idx]   = Vector3f(x, y, z);
            texcoords[idx] = Vector2f((float)seg / segments, (float)ring / rings);
            
            // Compute tangent from partial derivatives of sphere parameterization
            Vector3f tangent(-sinTheta, 0.0f, cosTheta);
            tangent.Normalize();
            tangents[idx] = Vector4f(tangent.x, tangent.y, tangent.z, 1.0f);
            
            ++idx;
        }
    }
    
    // Generate indices
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
    
    // Build Mesh object
    RenderSystem::MeshPtr mesh = std::make_shared<RenderSystem::Mesh>();
    
    RenderSystem::VertexData& vertexData = mesh->GetVertexData();
    uint32_t stride = sizeof(Vector3f) + sizeof(Vector4f) + sizeof(Vector3f) + sizeof(Vector2f); // pos(12) + tan(16) + norm(12) + uv(8) = 48
    vertexData.Resize(vertexCount, stride);
    
    RenderSystem::ChannelInfo* channels = vertexData.GetChannels();
    channels[RenderSystem::kShaderChannelPosition].offset  = 0;
    channels[RenderSystem::kShaderChannelPosition].format   = VertexFormatFloat3;
    channels[RenderSystem::kShaderChannelPosition].stride  = sizeof(Vector3f);
    
    channels[RenderSystem::kShaderChannelTangent].offset   = positions.size() * sizeof(Vector3f);
    channels[RenderSystem::kShaderChannelTangent].format   = VertexFormatFloat4;
    channels[RenderSystem::kShaderChannelTangent].stride   = 16;
    
    channels[RenderSystem::kShaderChannelNormal].offset     = positions.size() * sizeof(Vector3f) + tangents.size() * sizeof(Vector4f);
    channels[RenderSystem::kShaderChannelNormal].format     = VertexFormatFloat3;
    channels[RenderSystem::kShaderChannelNormal].stride     = 12;
    
    channels[RenderSystem::kShaderChannelTexCoord0].offset  = positions.size() * sizeof(Vector3f) + tangents.size() * sizeof(Vector4f) + normals.size() * sizeof(Vector3f);
    channels[RenderSystem::kShaderChannelTexCoord0].format  = VertexFormatFloat2;
    channels[RenderSystem::kShaderChannelTexCoord0].stride  = 8;
    
    mesh->SetPositions(positions.data(), vertexCount);
    mesh->SetTangents(tangents.data(), vertexCount);
    mesh->SetNormals(normals.data(), vertexCount);
    mesh->SetUv(0, texcoords.data(), vertexCount);
    mesh->SetIndices(indices.data(), indexCount);
    
    RenderSystem::SubMeshInfo subMeshInfo;
    subMeshInfo.firstIndex  = 0;
    subMeshInfo.indexCount  = indexCount;
    subMeshInfo.vertexCount = vertexCount;
    subMeshInfo.topology    = PrimitiveMode_TRIANGLES;
    mesh->AddSubMeshInfo(subMeshInfo);
    
    mesh->SetUpBuffer();
    
    return mesh;
}

// ============================================================
// Helper: Create a flat plane mesh for ground
// ============================================================
static RenderSystem::MeshPtr CreatePlaneMesh(float width, float depth, int xdivs, int zdivs)
{
    int nPoints = (xdivs + 1) * (zdivs + 1);
    std::vector<Vector3f> positions(nPoints);
    std::vector<Vector3f> normals(nPoints, Vector3f(0, 1, 0));
    std::vector<Vector4f> tangents(nPoints, Vector4f(1, 0, 0, 1));
    std::vector<Vector2f> texcoords(nPoints);
    std::vector<uint32_t> indices(xdivs * zdivs * 6);
    
    float halfW = width * 0.5f;
    float halfD = depth * 0.5f;
    
    int vidx = 0;
    for (int iz = 0; iz <= zdivs; ++iz)
    {
        float z = halfD - (float)iz / zdivs * depth;
        for (int ix = 0; ix <= xdivs; ++ix)
        {
            float x = -halfW + (float)ix / xdivs * width;
            positions[vidx]  = Vector3f(x, 0.0f, z);
            texcoords[vidx]  = Vector2f((float)ix / xdivs * 8.0f, (float)iz / zdivs * 8.0f);
            ++vidx;
        }
    }
    
    int iidx = 0;
    for (int iz = 0; iz < zdivs; ++iz)
    {
        int rowStart     = iz * (xdivs + 1);
        int nextRowStart = (iz + 1) * (xdivs + 1);
        for (int ix = 0; ix < xdivs; ++ix)
        {
            indices[iidx++] = rowStart + ix;
            indices[iidx++] = nextRowStart + ix;
            indices[iidx++] = nextRowStart + ix + 1;
            indices[iidx++] = rowStart + ix;
            indices[iidx++] = nextRowStart + ix + 1;
            indices[iidx++] = rowStart + ix + 1;
        }
    }
    
    RenderSystem::MeshPtr mesh = std::make_shared<RenderSystem::Mesh>();
    
    RenderSystem::VertexData& vertexData = mesh->GetVertexData();
    uint32_t stride = sizeof(Vector3f) + sizeof(Vector4f) + sizeof(Vector3f) + sizeof(Vector2f);
    vertexData.Resize(nPoints, stride);
    
    RenderSystem::ChannelInfo* channels = vertexData.GetChannels();
    channels[RenderSystem::kShaderChannelPosition].offset  = 0;
    channels[RenderSystem::kShaderChannelPosition].format   = VertexFormatFloat3;
    channels[RenderSystem::kShaderChannelPosition].stride  = sizeof(Vector3f);
    channels[RenderSystem::kShaderChannelTangent].offset   = positions.size() * sizeof(Vector3f);
    channels[RenderSystem::kShaderChannelTangent].format   = VertexFormatFloat4;
    channels[RenderSystem::kShaderChannelTangent].stride   = 16;
    channels[RenderSystem::kShaderChannelNormal].offset     = positions.size() * sizeof(Vector3f) + tangents.size() * sizeof(Vector4f);
    channels[RenderSystem::kShaderChannelNormal].format     = VertexFormatFloat3;
    channels[RenderSystem::kShaderChannelNormal].stride     = 12;
    channels[RenderSystem::kShaderChannelTexCoord0].offset  = positions.size() * sizeof(Vector3f) + tangents.size() * sizeof(Vector4f) + normals.size() * sizeof(Vector3f);
    channels[RenderSystem::kShaderChannelTexCoord0].format  = VertexFormatFloat2;
    channels[RenderSystem::kShaderChannelTexCoord0].stride  = 8;
    
    mesh->SetPositions(positions.data(), nPoints);
    mesh->SetTangents(tangents.data(), nPoints);
    mesh->SetNormals(normals.data(), nPoints);
    mesh->SetUv(0, texcoords.data(), nPoints);
    mesh->SetIndices(indices.data(), indices.size());
    
    RenderSystem::SubMeshInfo subMeshInfo;
    subMeshInfo.firstIndex  = 0;
    subMeshInfo.indexCount  = indices.size();
    subMeshInfo.vertexCount = nPoints;
    subMeshInfo.topology    = PrimitiveMode_TRIANGLES;
    mesh->AddSubMeshInfo(subMeshInfo);
    
    mesh->SetUpBuffer();
    
    return mesh;
}

// ============================================================
// Helper: Create a default sampler description (linear filter, repeat wrap)
// ============================================================
static RenderCore::SamplerDesc DefaultSamplerDesc()
{
    return RenderCore::SamplerDesc(
        RenderCore::MAG_LINEAR,
        RenderCore::MIN_LINEAR,
        RenderCore::REPEAT,
        RenderCore::REPEAT
    );
}

// ============================================================
// Helper: Load texture from file with fallback color texture
// ============================================================
static RenderCore::RCTexturePtr LoadTextureOrFallback(const char* filePath, float fr, float fg, float fb)
{
    RenderCore::RCTexturePtr tex = RenderSystem::ImageTextureUtil::TextureFromFile(filePath);
    if (!tex)
    {
        tex = RenderSystem::ImageTextureUtil::CreateDiffuseTexture(fr, fg, fb);
    }
    return tex;
}

// ============================================================
// Helper: Create a default gradient skybox (no external assets needed)
// Generates 6 faces with a subtle blue-to-horizon gradient
// ============================================================
static RenderSystem::SkyBox* CreateDefaultSkybox()
{
    using namespace imagecodec;
    
    const uint32_t faceSize = 256; // 每面分辨率（天空盒不需要太高）
    auto renderDevice = RenderCore::GetRenderDevice();
    
    // 为6个面生成渐变图像
    std::vector<VImagePtr> faceImages(6);
    
    for (int face = 0; face < 6; ++face)
    {
        uint8_t* data = (uint8_t*)malloc(faceSize * faceSize * 4); // RGBA8
        
        for (uint32_t y = 0; y < faceSize; ++y)
        {
            for (uint32_t x = 0; x < faceSize; ++x)
            {
                float u = (float)x / (float)(faceSize - 1);
                float v = (float)y / (float)(faceSize - 1);
                
                // 根据面的不同计算不同的渐变
                // +X(0), -X(1), +Y(2=top), -Y(3=bottom), +Z(4), -Z(5)
                Vector3f color;
                
                if (face == 2) // top: 天顶偏蓝
                {
                    color = Vector3f(0.15f, 0.35f, 0.75f) * (1.0f - v) + Vector3f(0.45f, 0.60f, 0.85f) * v;
                }
                else if (face == 3) // bottom: 地平线暗色
                {
                    color = Vector3f(0.10f, 0.12f, 0.18f);
                }
                else // 四周: 地平线到天际的垂直渐变
                {
                    // 垂直方向：底部暗 → 中间亮蓝 → 顶部深蓝
                    float t = v;
                    if (t < 0.5f)
                    {
                        float s = t * 2.0f;
                        color = Vector3f(0.08f, 0.10f, 0.16f) * (1.0f - s) + Vector3f(0.35f, 0.50f, 0.72f) * s;
                    }
                    else
                    {
                        float s = (t - 0.5f) * 2.0f;
                        color = Vector3f(0.35f, 0.50f, 0.72f) * (1.0f - s) + Vector3f(0.12f, 0.25f, 0.55f) * s;
                    }
                    
                    // 水平方向加微妙变化
                    float hGrad = fabs(u - 0.5f) * 2.0f;
                    color = color * (1.0f - hGrad * 0.08f);
                }
                
                uint32_t idx = (y * faceSize + x) * 4;
                data[idx + 0] = (uint8_t)(std::min(std::max(color.x, 0.0f), 1.0f) * 255.0f); // R
                data[idx + 1] = (uint8_t)(std::min(std::max(color.y, 0.0f), 1.0f) * 255.0f); // G
                data[idx + 2] = (uint8_t)(std::min(std::max(color.z, 0.0f), 1.0f) * 255.0f); // B
                data[idx + 3] = 255; // A
            }
        }
        
        faceImages[face] = std::make_shared<VImage>();
        faceImages[face]->SetImageInfo(FORMAT_RGBA8, faceSize, faceSize, data, free);
    }
    
    // 使用 VImagePtr 版本的 create 工厂方法
    return RenderSystem::SkyBox::create(renderDevice,
        faceImages[0], faceImages[1], // +X, -X
        faceImages[2], faceImages[3], // +Y (top), -Y (bottom)
        faceImages[4], faceImages[5]); // +Z, -Z
}

// ============================================================
// Implementation
// ============================================================

PBRFrameWork::PBRFrameWork(const GNXEngine::WindowProps& props) 
    : GNXEngine::AppFrameWork(props)
{
}

void PBRFrameWork::Initlize()
{
    GNXEngine::AppFrameWork::Initlize();
}

void PBRFrameWork::Resize(uint32_t width, uint32_t height)
{
    AppFrameWork::Resize(width, height);
    
    RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();
    
    // ---- Camera Setup ----
    RenderSystem::CameraPtr cameraPtr = sceneManager->GetCamera("MainCamera");
    if (!cameraPtr)
    {
        cameraPtr = sceneManager->CreateCamera("MainCamera");
    }
    cameraPtr->LookAt(
        Vector3f(0.0f, 2.5f, 6.0f),
        Vector3f(0.0f, 0.0f, 0.0f),
        Vector3f(0.0f, 1.0f, 0.0f)
    );
    cameraPtr->SetLens(60, width, height, 0.1f, 100.0f);
    
    // ---- Lighting Setup ----
    
    // Primary directional light (sun/key light)
    RenderSystem::DirectionLight* dirLight = static_cast<RenderSystem::DirectionLight*>(
        sceneManager->CreateLight("sun", RenderSystem::Light::DirectionLight));
    dirLight->setColor(Vector3f(1.0f, 0.95f, 0.9f));       // warm white key light
    dirLight->setDirection(Vector3f(-0.6f, -0.7f, -0.4f).Normalize());
    dirLight->setStrength(Vector3f(3.0f, 3.0f, 3.0f));      // 光源强度
    
    // ---- Common PBR textures (default fallbacks) ----
    RenderCore::SamplerDesc sampDesc = DefaultSamplerDesc();
    
    // ============================================================
    // BRDF LUT 预计算（Split-Sum 近似预积分表）
    // 只需在初始化时计算一次，后续所有帧复用
    // ============================================================
    mBRDFLUT = RenderSystem::ImageTextureUtil::CreateBRDFLUTTexture(512, 1024);
    if (!mBRDFLUT)
    {
        LOG_ERROR("Failed to create BRDF LUT texture, IBL specular will be disabled");
    }
    else
    {
        // 将 BRDF LUT 注册到延迟渲染管线
        RenderSystem::SceneManager* sceneManager = RenderSystem::SceneManager::GetInstance();
        sceneManager->SetIBLTextures(/*irradiance=*/nullptr, /*prefiltered=*/nullptr, mBRDFLUT);
    }
    
    // ==================================================
    // Skybox: 创建默认渐变天空盒（无外部资源依赖）
    // 后续可替换为真实 HDR 环境贴图
    // ==================================================
    mSkyBox = CreateDefaultSkybox();
    if (mSkyBox)
    {
        sceneManager->GetSkyBox()->AttachSkyBoxObject(mSkyBox);
        LOG_INFO("Default skybox created and attached");
    }
    
    RenderCore::RCTexturePtr defaultAlbedo      = RenderSystem::ImageTextureUtil::CreateDiffuseTexture(0.8f, 0.8f, 0.8f);
    RenderCore::RCTexturePtr defaultNormal       = RenderSystem::ImageTextureUtil::CreateNormalTexture();
    RenderCore::RCTexturePtr defaultMetalRough   = RenderSystem::ImageTextureUtil::CreateMetalRoughTexture();
    RenderCore::RCTexturePtr defaultAO           = RenderSystem::ImageTextureUtil::CreateAOTexture();
    RenderCore::RCTexturePtr defaultEmissive     = RenderSystem::ImageTextureUtil::CreateEmmisveTexture();
    
    // ==================================================
    // Scene Object 1: DamagedHelmet Model (full PBR textures)
    // ==================================================
    {
        std::string helmetPath = GetProjectAssetDir() + "pbr/DamagedHelmet/glTF/DamagedHelmet.gltf";
        
        // Place helmet on a pedestal position
        Matrix4x4f translateMatrix = Matrix4x4f::CreateTranslate(0.0f, 1.3f, -2.0f);
        Matrix4x4f scaleMatrix     = Matrix4x4f::CreateScale(1.8f, 1.8f, 1.8f);
        Matrix4x4f rotateMatrix   = Matrix4x4f::CreateRotation(0, 1, 0, -45.0f);
        Matrix4x4f modelMatrix    = scaleMatrix;
        
        RenderSystem::Transform transform;
        transform.TransformFromMat4(modelMatrix);
        
        RenderSystem::SceneNode* helmetNode = sceneManager->GetRootNode()->CreateRendererNode(
            "DamagedHelmet", helmetPath, transform.position, transform.rotation, transform.scale);
        
        // The model loader auto-populates materials from glTF.
        // We can override/enhance them here if needed:
        RenderSystem::MeshRenderer* meshRender = helmetNode->QueryComponentT<RenderSystem::MeshRenderer>();
        if (meshRender && !meshRender->GetMaterials().empty())
        {
            const std::vector<RenderSystem::MaterialPtr>& mats = meshRender->GetMaterials();
            // Materials are already loaded from glTF with PBR properties
        }
    }
}

void PBRFrameWork::RenderFrame()
{
    using namespace RenderCore;
    using namespace RenderSystem;
    
    static uint64_t lastTime = 0;
    uint64_t thisTime = baselib::GetTickNanoSeconds();
    float deltaTime = float(thisTime - lastTime) * 0.000000001f;
    lastTime = thisTime;
    
    SceneManager* sceneManager = SceneManager::GetInstance();
    sceneManager->Update(deltaTime);
    
    // Render via DeferredSceneRenderer (G-Buffer → Deferred Lighting pipeline)
    sceneManager->Render(nullptr);
}

void PBRFrameWork::OnEvent(GNXEngine::Event& e)
{
    GNXEngine::AppFrameWork::OnEvent(e);
    RenderSystem::SceneManager::GetInstance()->OnEvent(e);
    
    GNXEngine::EventDispatcher dispatcher(e);
    dispatcher.Dispatch<GNXEngine::KeyReleasedEvent>(GNX_BIND_EVENT_FN(OnKeyUp));
}

bool PBRFrameWork::OnKeyUp(GNXEngine::KeyReleasedEvent& e)
{
    return true;
}
