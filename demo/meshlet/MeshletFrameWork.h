//
//  MeshletFrameWork.h
//  meshlet
//
//  Meshlet Demo - Loads .meshlet binary file and renders via Mesh Shaders.
//  Each mesh workgroup processes one meshlet using vertex+index SSBOs.
//

#ifndef MeshletFrameWork_h
#define MeshletFrameWork_h

#include "Runtime/GNXEngine/include/AppFrameWork.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"
#include "Runtime/RenderCore/include/UniformBuffer.h"
#include "Runtime/RenderCore/include/RCBuffer.h"
#include "Runtime/RenderSystem/include/meshlet/MeshLetCommon.h"
#include "Runtime/RenderSystem/include/meshlet/MeshLetFile.h"
#include "Runtime/RenderSystem/include/RenderParameter.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include "Runtime/MathUtil/include/AABB.h"

class MeshletFrameWork : public GNXEngine::AppFrameWork
{
public:
    MeshletFrameWork(const GNXEngine::WindowProps& props);

    virtual void Initlize() override;
    virtual void Resize(uint32_t width, uint32_t height) override;
    virtual void RenderFrame() override;
    virtual void OnEvent(GNXEngine::Event& e) override;

private:
    void CreatePipeline();
    void LoadMeshletData();
    void InitCullingData();

    RenderCore::RenderDevicePtr mRenderDevice = nullptr;
    RenderCore::GraphicsPipelinePtr mMeshPipeline = nullptr;

    // SSBOs uploaded from .meshlet file
    RenderCore::RCBufferPtr mMeshletDescSSBO = nullptr;    // meshlet descriptors (StructuredBuffer<Meshlet>)
    RenderCore::RCBufferPtr mMeshletVertsSSBO = nullptr;   // meshlet vertex indices (uint32_t[])
    RenderCore::RCBufferPtr mMeshletTriSSBO = nullptr;     // meshlet triangle indices (uint32_t[], packed 3 uint8 per uint32)
    RenderCore::RCBufferPtr mVertexPosSSBO = nullptr;      // vertex positions (float3[])

    // Culling data SSBO
    RenderCore::RCBufferPtr mMeshletBoundsSSBO = nullptr;  // bounding spheres (float4 per meshlet)
    
    RenderCore::RCBufferPtr mInstanceSSBO = nullptr;  // model matrix instance

    // per-object uniform buffer (contains model matrix for shader's cbPerObject)
    RenderCore::UniformBufferPtr mPerObjectUBO = nullptr;

    // cbMeshletParams UBO (gInstanceCount, gMeshletCount → Task Shader)
    RenderCore::UniformBufferPtr mMeshletParamsUBO = nullptr;

    RenderSystem::MeshletFileData mMeshletData;

    mathutil::AxisAlignedBoxf mMeshAABB;              // 整个模型的 AABB

    uint32_t mMeshletCount = 0;
    uint32_t mInstancesCount = 0;
    uint32_t mVertexCount  = 0;
    uint32_t mWidth  = 1280;
    uint32_t mHeight = 720;
};

#endif /* MeshletFrameWork_h */
