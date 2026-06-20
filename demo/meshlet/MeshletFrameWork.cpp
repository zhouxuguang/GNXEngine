//
//  MeshletFrameWork.cpp
//  meshlet
//
//  Demonstrates loading a pre-built .meshlet file and rendering via mesh shaders.
//
//  Pipeline:
//    MS: reads Meshlet descriptor + vertex positions + packed triangle indices
//        outputs triangles with per-vertex normals (flat shaded)
//    FS: simple color output
//

#include "MeshletFrameWork.h"
#include "Runtime/RenderCore/include/RenderEncoder.h"
#include "Runtime/RenderCore/include/CommandBuffer.h"
#include "Runtime/RenderCore/include/CommandQueue.h"
#include "Runtime/RenderCore/include/ShaderFunction.h"
#include "Runtime/RenderCore/include/RenderDefine.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/RenderSystem/include/ShaderAssetLoader.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/BaseLib/include/FileUtil.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/RenderSystem/include/SceneManager.h"

using namespace RenderCore;
using namespace RenderSystem;
using namespace mathutil;

struct UniformData
{
    Matrix4x4f projection;
    Matrix4x4f model;
    Matrix4x4f view;
};

MeshletFrameWork::MeshletFrameWork(const GNXEngine::WindowProps& props)
    : GNXEngine::AppFrameWork(props)
{
    mWidth  = props.width;
    mHeight = props.height;
}

void MeshletFrameWork::Initlize()
{
    GNXEngine::AppFrameWork::Initlize();
    mRenderDevice = GetRenderDevice();

    LoadMeshletData();
    InitCullingData();
    CreatePipeline();

    // Create per-object UBO for cbPerObject (model matrix)
    mPerObjectUBO = mRenderDevice->CreateUniformBufferWithSize(sizeof(RenderSystem::cbPerObject));
}

void MeshletFrameWork::LoadMeshletData()
{
    // Use standard project asset directory (consistent with other demos)
    std::string meshletPath = GetProjectAssetDir() + "meshlet/horse_statue_01_1k.meshlet";

    if (!LoadMeshletFile(meshletPath, mMeshletData))
    {
        LOG_ERROR("Failed to load .meshlet file: %s. Use meshlet_gen tool first.", meshletPath.c_str());
        return;
    }

    mMeshletCount = mMeshletData.GetMeshletCount();
    mVertexCount  = mMeshletData.GetVertexCount();

    LOG_INFO("Loaded meshlet: %u vertices, %u meshlets",
             mVertexCount, mMeshletCount);
}

void MeshletFrameWork::InitCullingData()
{
    if (mVertexCount == 0) return;

    // ---- 1. Vertex positions SSBO ----
    {
        size_t posBytes = mMeshletData.vertexPositions.size() * sizeof(float);
        RCBufferDesc desc((uint32_t)posBytes, RCBufferUsage::StorageBuffer, StorageModeShared);
        mVertexPosSSBO = mRenderDevice->CreateBuffer(desc);
        mVertexPosSSBO->SetName("Meshlet_Vertices");
        void* dst = mVertexPosSSBO->Map();
        if (dst)
        {
            memcpy(dst, mMeshletData.vertexPositions.data(), posBytes);
            mVertexPosSSBO->Unmap();
        }
    }

    // ---- 2. Meshlet descriptor SSBO ----
    {
        size_t descBytes = mMeshletCount * sizeof(Meshlet);
        RCBufferDesc desc((uint32_t)descBytes, RCBufferUsage::StorageBuffer, StorageModeShared);
        mMeshletDescSSBO = mRenderDevice->CreateBuffer(desc);
        mMeshletDescSSBO->SetName("Meshlet_Descs");
        void* dst = mMeshletDescSSBO->Map();
        if (dst)
        {
            memcpy(dst, mMeshletData.meshlets.data(), descBytes);
            mMeshletDescSSBO->Unmap();
        }
    }

    // ---- 3. Meshlet vertex indices SSBO (uint32_t[]) ----
    {
        size_t vertIdxBytes = mMeshletData.meshletVertices.size() * sizeof(uint32_t);
        RCBufferDesc desc((uint32_t)vertIdxBytes, RCBufferUsage::StorageBuffer, StorageModeShared);
        mMeshletVertsSSBO = mRenderDevice->CreateBuffer(desc);
        mMeshletVertsSSBO->SetName("Meshlet_VertIndices");
        void* dst = mMeshletVertsSSBO->Map();
        if (dst)
        {
            memcpy(dst, mMeshletData.meshletVertices.data(), vertIdxBytes);
            mMeshletVertsSSBO->Unmap();
        }
    }

    // ---- 4. Meshlet triangle indices SSBO (uint32_t[], packed - 3 uint8 indices per uint32) ----
    {
        size_t triBytes = mMeshletData.meshletTriangles.size() * sizeof(uint32_t);
        RCBufferDesc desc((uint32_t)triBytes, RCBufferUsage::StorageBuffer, StorageModeShared);
        mMeshletTriSSBO = mRenderDevice->CreateBuffer(desc);
        mMeshletTriSSBO->SetName("Meshlet_TriIndices");
        void* dst = mMeshletTriSSBO->Map();
        if (dst)
        {
            memcpy(dst, mMeshletData.meshletTriangles.data(), triBytes);
            mMeshletTriSSBO->Unmap();
        }
    }

    // ---- 5. Bounding sphere SSBO (float4 per meshlet) ----
    {
        std::vector<Vector4f> bounds(mMeshletCount);
        for (uint32_t i = 0; i < mMeshletCount; ++i)
        {
            const auto& m = mMeshletData.meshlets[i];
            Vector3f center;
            float radius = 0.0f;
            ComputeMeshletBounds(
                mMeshletData.vertexPositions.data(),
                                 mMeshletData.meshletVertices.data() + m.vertexOffset,
                                 m.vertexCount,
                center, radius);
            bounds[i] = Vector4f(center.x, center.y, center.z, radius);
        }
        RCBufferDesc desc((uint32_t)(mMeshletCount * sizeof(Vector4f)), RCBufferUsage::StorageBuffer, StorageModeShared);
        mMeshletBoundsSSBO = mRenderDevice->CreateBuffer(desc);
        mMeshletBoundsSSBO->SetName("Meshlet_Bounds");
        void* dst = mMeshletBoundsSSBO->Map();
        if (dst)
        {
            memcpy(dst, bounds.data(), mMeshletCount * sizeof(Vector4f));
            mMeshletBoundsSSBO->Unmap();
        }
    }

    LOG_INFO("Uploaded %u meshlet descriptors, %zu vertex indices, %zu packed triangles",
             mMeshletCount,
             mMeshletData.meshletVertices.size() / 64 * 64,
             mMeshletData.meshletTriangles.size());
}

void MeshletFrameWork::CreatePipeline()
{
    if (!mRenderDevice || mVertexCount == 0) return;

    RenderSystem::GraphicsShaderInfo shaderInfo =
        RenderSystem::CreateGraphicsShaderInfo("MeshShader/MeshletDemo");

    if (!shaderInfo.graphicsShader)
    {
        LOG_ERROR("Failed to create meshlet graphics shader");
        return;
    }

    // color attachment
    shaderInfo.graphicsPipelineDesc.renderTargetCount = 1;
    shaderInfo.graphicsPipelineDesc.colorAttachmentDescriptors[0].blendingEnabled = false;
    shaderInfo.graphicsPipelineDesc.colorAttachmentDescriptors[0].writeMask = ColorWriteMaskAll;

    // depth
    shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthCompareFunction = CompareFunctionGreaterThanOrEqual;
    shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = true;

    mMeshPipeline = mRenderDevice->CreateGraphicsPipeline(shaderInfo.graphicsPipelineDesc);
    if (!mMeshPipeline)
    {
        LOG_ERROR("Failed to create meshlet pipeline");
        return;
    }
    mMeshPipeline->AttachGraphicsShader(shaderInfo.graphicsShader);

    LOG_INFO("Meshlet mesh shader pipeline created successfully");
}

void MeshletFrameWork::Resize(uint32_t width, uint32_t height)
{
    AppFrameWork::Resize(width, height);
    mWidth  = width;
    mHeight = height;
    
    RenderSystem::SceneManager *sceneManager = RenderSystem::SceneManager::GetInstance();

    RenderSystem::CameraPtr cameraPtr = sceneManager->GetCamera("MainCamera");
    if (!cameraPtr)
    {
        cameraPtr = sceneManager->CreateCamera("MainCamera");
    }

    cameraPtr->LookAt(mathutil::Vector3f(0, 0.105f, 0.40f), mathutil::Vector3f(0, 0.105f, 0), mathutil::Vector3f(0, 1, 0));
    cameraPtr->SetLens(60, width, height, 0.1f, 1000.f);
}

void MeshletFrameWork::RenderFrame()
{
    if (!mRenderDevice || !mMeshPipeline || mMeshletCount == 0)
    {
        return;
    }

    static uint64_t lastTime = 0;
    uint64_t thisTime = baselib::GetTickNanoSeconds();
    float deltaTime = float(thisTime - lastTime) * 0.000000001f;
    lastTime = thisTime;
    LOG_INFO("deltaTime = %f\n", deltaTime);

    RenderSystem::SceneManager *sceneManager = RenderSystem::SceneManager::GetInstance();
    sceneManager->Update(deltaTime);

    // ---- Render ----
    CommandQueuePtr graphicsQueue = mRenderDevice->GetCommandQueue(QueueType::Graphics, 0);
    CommandBufferPtr commandBuffer = graphicsQueue->CreateCommandBuffer();

    {
        RenderEncoderPtr renderEncoder = commandBuffer->CreateDefaultRenderEncoder();
        if (!renderEncoder) return;

        renderEncoder->SetGraphicsPipeline(mMeshPipeline);
        
        // Bind camera UBO from SceneManager (cbPerCamera)
        RenderSystem::RenderInfo renderInfo = sceneManager->GetRenderInfo();
        renderEncoder->SetMeshUniformBuffer("cbPerCamera", renderInfo.cameraUBO);

        // Fill and bind per-object UBO (cbPerObject: model matrix)
        {
            RenderSystem::cbPerObject objData;
            objData.MATRIX_M.MakeIdentity();
            objData.MATRIX_M_INV.MakeIdentity();
            objData.MATRIX_Normal.MakeIdentity();
            mPerObjectUBO->SetData(&objData, 0, sizeof(RenderSystem::cbPerObject));
        }
        renderEncoder->SetMeshUniformBuffer("cbPerObject", mPerObjectUBO);

        // bind SSBOs
        if (mVertexPosSSBO)
            renderEncoder->SetStorageBuffer("Vertices", mVertexPosSSBO, ShaderStage_Mesh);
        if (mMeshletDescSSBO)
            renderEncoder->SetStorageBuffer("Meshlets", mMeshletDescSSBO, ShaderStage_Mesh);
        if (mMeshletVertsSSBO)
            renderEncoder->SetStorageBuffer("VertexIndices", mMeshletVertsSSBO, ShaderStage_Mesh);
        if (mMeshletTriSSBO)
            renderEncoder->SetStorageBuffer("TriangleIndices", mMeshletTriSSBO, ShaderStage_Mesh);

        // Dispatch one task group per meshlet
        renderEncoder->DrawMeshTasks(mMeshletCount, 1, 1);

        renderEncoder->EndEncode();
    }

    commandBuffer->PresentFrameBuffer();
}

void MeshletFrameWork::OnEvent(GNXEngine::Event& e)
{
    GNXEngine::AppFrameWork::OnEvent(e);
}
