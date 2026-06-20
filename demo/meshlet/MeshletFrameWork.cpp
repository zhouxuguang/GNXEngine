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
}

void MeshletFrameWork::LoadMeshletData()
{
    // Look for .meshlet file in data_asset/meshlet/
    std::string meshletPath;
    std::vector<std::string> candidates = {
        "data_asset/meshlet/horse_statue_01_1k.meshlet",
        "../../../data_asset/meshlet/horse_statue_01_1k.meshlet",
        "assets/meshlet/horse_statue_01_1k.meshlet"
    };
    for (const auto& p : candidates)
    {
        if (LoadMeshletFile(p, mMeshletData))
        {
            meshletPath = p;
            break;
        }
    }

    if (mMeshletData.GetVertexCount() == 0)
    {
        LOG_ERROR("Failed to load any .meshlet file. Use meshlet_gen tool first.");
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
    shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthCompareFunction = CompareFunctionLessThanOrEqual;
    shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = true;

    mMeshPipeline = mRenderDevice->CreateGraphicsPipeline(shaderInfo.graphicsPipelineDesc);
    if (!mMeshPipeline)
    {
        LOG_ERROR("Failed to create meshlet pipeline");
        return;
    }
    mMeshPipeline->AttachGraphicsShader(shaderInfo.graphicsShader);

    // uniform buffer
    mUniformBuffer = mRenderDevice->CreateUniformBufferWithSize(sizeof(UniformData));

    LOG_INFO("Meshlet mesh shader pipeline created successfully");
}

void MeshletFrameWork::Resize(uint32_t width, uint32_t height)
{
    AppFrameWork::Resize(width, height);
    mWidth  = width;
    mHeight = height;
}

void MeshletFrameWork::RenderFrame()
{
    if (!mRenderDevice || !mMeshPipeline || mMeshletCount == 0)
    {
        return;
    }

    // ---- Update UBO ----
    UniformData uboData;
    uboData.model.MakeIdentity();
    uboData.view = Matrix4x4f::CreateLookAt(
        Vector3f(0.0f, 2.0f, -5.0f),
        Vector3f(0.0f, 1.0f, 0.0f),
        Vector3f(0.0f, 1.0f, 0.0f));
    uboData.projection = Matrix4x4f::CreatePerspective(
        60.0f, (float)mWidth / (float)mHeight, 0.1f, 100.0f);
    mUniformBuffer->SetData(&uboData, 0, sizeof(UniformData));

    // ---- Render ----
    CommandQueuePtr graphicsQueue = mRenderDevice->GetCommandQueue(QueueType::Graphics, 0);
    CommandBufferPtr commandBuffer = graphicsQueue->CreateCommandBuffer();

    {
        RenderEncoderPtr renderEncoder = commandBuffer->CreateDefaultRenderEncoder();
        if (!renderEncoder) return;

        renderEncoder->SetGraphicsPipeline(mMeshPipeline);

        // bind UBO to mesh shader (binding 0)
        renderEncoder->SetMeshUniformBuffer(mUniformBuffer, 0);

        // bind SSBOs
        if (mVertexPosSSBO)
            renderEncoder->SetStorageBuffer("gVertexPositions", mVertexPosSSBO, ShaderStage_Mesh);
        if (mMeshletDescSSBO)
            renderEncoder->SetStorageBuffer("gMeshlets", mMeshletDescSSBO, ShaderStage_Mesh);
        if (mMeshletVertsSSBO)
            renderEncoder->SetStorageBuffer("gMeshletVertices", mMeshletVertsSSBO, ShaderStage_Mesh);
        if (mMeshletTriSSBO)
            renderEncoder->SetStorageBuffer("gMeshletTriangles", mMeshletTriSSBO, ShaderStage_Mesh);
        if (mMeshletBoundsSSBO)
            renderEncoder->SetStorageBuffer("gMeshletBounds", mMeshletBoundsSSBO, ShaderStage_Mesh);

        // Dispatch one task group per meshlet
        const uint32_t* groupSizes = mMeshPipeline->GetMeshThreadgroupSize();
        uint32_t dispatchX = (mMeshletCount + groupSizes[0] - 1) / groupSizes[0];
        renderEncoder->DrawMeshTasks(dispatchX, 1, 1);

        renderEncoder->EndEncode();
    }

    commandBuffer->PresentFrameBuffer();
}

void MeshletFrameWork::OnEvent(GNXEngine::Event& e)
{
    GNXEngine::AppFrameWork::OnEvent(e);
}
