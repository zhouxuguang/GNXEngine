//
//  MeshShaderFrameWork.cpp
//  meshshader
//
//  Mesh Shader Demo - Displays 3 overlapping colored triangles
//  using task (amplification) + mesh shaders.
//

#include "MeshShaderFrameWork.h"
#include "Runtime/RenderCore/include/RenderEncoder.h"
#include "Runtime/RenderCore/include/CommandBuffer.h"
#include "Runtime/RenderCore/include/CommandQueue.h"
#include "Runtime/RenderCore/include/ShaderFunction.h"
#include "Runtime/RenderCore/include/RenderDefine.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/RenderSystem/include/ShaderAssetLoader.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include "Runtime/BaseLib/include/LogService.h"

using namespace RenderCore;
using namespace mathutil;

struct UniformData
{
    Matrix4x4f projection;
    Matrix4x4f model;
    Matrix4x4f view;
};

MeshShaderFrameWork::MeshShaderFrameWork(const GNXEngine::WindowProps& props)
    : GNXEngine::AppFrameWork(props)
{
    mWidth = props.width;
    mHeight = props.height;
}

void MeshShaderFrameWork::Initlize()
{
    GNXEngine::AppFrameWork::Initlize();
    mRenderDevice = GetRenderDevice();
    CreatePipeline();
    CreateVertexSSBO();
}

void MeshShaderFrameWork::CreatePipeline()
{
    if (!mRenderDevice)
    {
        return;
    }

    // 使用统一的 CreateGraphicsShaderInfo 接口
    // 会自动检测 Mesh Shader 并设置 PipelineType::Mesh
    RenderSystem::GraphicsShaderInfo shaderInfo = RenderSystem::CreateGraphicsShaderInfo("MeshShader/MeshShaderDemo");

    if (!shaderInfo.graphicsShader)
    {
        LOG_ERROR("Failed to create mesh graphics shader");
        return;
    }

    // 补充 pipeline 描述
    shaderInfo.graphicsPipelineDesc.maxObjectPayloadMeshlets = 3;

    // color attachment (default RGBA8)
    shaderInfo.graphicsPipelineDesc.renderTargetCount = 1;
    shaderInfo.graphicsPipelineDesc.colorAttachmentDescriptors[0].blendingEnabled = false;
    shaderInfo.graphicsPipelineDesc.colorAttachmentDescriptors[0].writeMask = ColorWriteMaskAll;

    // depth attachment
    shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthCompareFunction = CompareFunctionLessThanOrEqual;
    shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = true;

    // create pipeline
    mMeshPipeline = mRenderDevice->CreateGraphicsPipeline(shaderInfo.graphicsPipelineDesc);
    if (!mMeshPipeline)
    {
        LOG_ERROR("Failed to create mesh pipeline");
        return;
    }

    // 使用 AttachGraphicsShader 绑定 shader（支持 Mesh Shader 的 TS+MS+FS 组合）
    mMeshPipeline->AttachGraphicsShader(shaderInfo.graphicsShader);

    // create uniform buffer (3 mat4 = 192 bytes)
    mUniformBuffer = mRenderDevice->CreateUniformBufferWithSize(sizeof(UniformData));
    if (!mUniformBuffer)
    {
        LOG_ERROR("Failed to create uniform buffer");
        return;
    }

    LOG_INFO("Mesh shader pipeline created successfully");
}

void MeshShaderFrameWork::CreateVertexSSBO()
{
    if (!mRenderDevice)
    {
        return;
    }

    // 2 triangles × 3 vertices each = 6 vertices
    // Triangle 0: green-cyan-blue (left side)
    // Triangle 1: orange-yellow-red (right side, offset +X)
    constexpr uint32_t kVertexCount = 6;
    constexpr uint32_t kBufferSize = kVertexCount * sizeof(SSBOVertexData);

    RCBufferDesc desc(kBufferSize,
        RCBufferUsage::StorageBuffer,
        StorageModeShared);
    mVertexSSBO = mRenderDevice->CreateBuffer(desc);
    if (!mVertexSSBO)
    {
        LOG_ERROR("Failed to create vertex SSBO");
        return;
    }
    mVertexSSBO->SetName("MeshDemo_VertexSSBO");

    // Fill vertex data
    SSBOVertexData* vertices = static_cast<SSBOVertexData*>(mVertexSSBO->Map());
    if (!vertices)
    {
        LOG_ERROR("Failed to map vertex SSBO");
        return;
    }

    // Triangle 0: left triangle (green → cyan → blue)
    vertices[0] = {{ -0.5f,  0.5f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}};   // top,    green
    vertices[1] = {{ -1.0f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}};   // bottom-left, cyan
    vertices[2] = {{  0.0f, -0.5f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}};   // bottom-right, blue

    // Triangle 1: right triangle (orange → yellow → red), offset by +1.0 in X
    vertices[3] = {{  0.5f,  0.5f, 0.0f, 1.0f}, {1.0f, 0.5f, 0.0f, 1.0f}};   // top,    orange
    vertices[4] = {{  0.0f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}};   // bottom-left, yellow
    vertices[5] = {{  1.0f, -0.5f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}};   // bottom-right, red

    mVertexSSBO->Unmap();

    LOG_INFO("Mesh shader vertex SSBO created: %u vertices (%u bytes)",
             kVertexCount, kBufferSize);
}

void MeshShaderFrameWork::Resize(uint32_t width, uint32_t height)
{
    AppFrameWork::Resize(width, height);
    mWidth = width;
    mHeight = height;
}

void MeshShaderFrameWork::RenderFrame()
{
    if (!mRenderDevice || !mMeshPipeline)
    {
        return;
    }

    // update uniform data
    UniformData uboData;
    uboData.model = Matrix4x4f();
    uboData.model.MakeIdentity();
    uboData.view = Matrix4x4f::CreateLookAt(
        Vector3f(0.0f, 0.0f, -5.0f),
        Vector3f(0.0f, 0.0f, 0.0f),
        Vector3f(0.0f, 1.0f, 0.0f)
    );
    uboData.view.MakeIdentity();
    uboData.projection = Matrix4x4f::CreatePerspective(60.0f, (float)mWidth / (float)mHeight, 0.1f, 100.0f);
    uboData.projection.MakeIdentity();
    mUniformBuffer->SetData(&uboData, 0, sizeof(UniformData));

    // create command buffer
    CommandQueuePtr graphicsQueue = mRenderDevice->GetCommandQueue(QueueType::Graphics, 0);
    CommandBufferPtr commandBuffer = graphicsQueue->CreateCommandBuffer();
    if (!commandBuffer)
    {
        return;
    }

    // create render encoder
    RenderEncoderPtr renderEncoder = commandBuffer->CreateDefaultRenderEncoder();
    if (!renderEncoder)
    {
        return;
    }

    // set mesh pipeline
    renderEncoder->SetGraphicsPipeline(mMeshPipeline);

    // bind uniform buffer to mesh shader (binding 0)
    renderEncoder->SetMeshUniformBuffer(mUniformBuffer, 0);

    // bind vertex SSBO to mesh shader stage (StructuredBuffer read in MS)
    if (mVertexSSBO)
    {
        renderEncoder->SetStorageBuffer("gVertices", mVertexSSBO, ShaderStage_Mesh);
    }

    // draw: 2 object threadgroups -> each dispatches 1 mesh threadgroup -> 2 triangles
    // Triangle 0 from vertices[0..2], Triangle 1 from vertices[3..5]
    renderEncoder->DrawMeshTasks(2, 1, 1);

    renderEncoder->EndEncode();
    commandBuffer->PresentFrameBuffer();
}

void MeshShaderFrameWork::OnEvent(GNXEngine::Event& e)
{
    GNXEngine::AppFrameWork::OnEvent(e);
}
