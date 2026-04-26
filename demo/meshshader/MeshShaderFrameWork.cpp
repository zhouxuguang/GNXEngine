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
#include "Runtime/ShaderCompiler/include/ShaderCompiler.h"
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
}

void MeshShaderFrameWork::CreatePipeline()
{
    if (!mRenderDevice)
    {
        return;
    }

    RenderDeviceType renderType = mRenderDevice->GetRenderDeviceType();
    ShaderFunctionPtr taskShader;
    ShaderFunctionPtr meshShader;
    ShaderFunctionPtr fragmentShader;

    std::string shaderDir = getBuiltInShaderDir();
    std::string shaderFilePath = shaderDir + "MeshShader/MeshShaderDemo.shader";

    shader_compiler::CompiledShaderInfoPtr taskShaderInfo =
        shader_compiler::CompileShader(shaderFilePath, ShaderStage_Task, renderType);
    if (!taskShaderInfo || !taskShaderInfo->shaderSource)
    {
        LOG_ERROR("Failed to compile task shader");
        return;
    }

    shader_compiler::CompiledShaderInfoPtr meshShaderInfo =
        shader_compiler::CompileShader(shaderFilePath, ShaderStage_Mesh, renderType);
    if (!meshShaderInfo || !meshShaderInfo->shaderSource)
    {
        LOG_ERROR("Failed to compile mesh shader");
        return;
    }

    shader_compiler::CompiledShaderInfoPtr fragmentShaderInfo =
        shader_compiler::CompileShader(shaderFilePath, ShaderStage_Fragment, renderType);
    if (!fragmentShaderInfo || !fragmentShaderInfo->shaderSource)
    {
        LOG_ERROR("Failed to compile fragment shader");
        return;
    }

    taskShader = mRenderDevice->CreateShaderFunction(
        *taskShaderInfo->shaderSource, ShaderStage_Task);
    meshShader = mRenderDevice->CreateShaderFunction(
        *meshShaderInfo->shaderSource, ShaderStage_Mesh);
    fragmentShader = mRenderDevice->CreateShaderFunction(
        *fragmentShaderInfo->shaderSource, ShaderStage_Fragment);

    if (!taskShader || !meshShader || !fragmentShader)
    {
        LOG_ERROR("Failed to create shader functions");
        return;
    }

    // create mesh pipeline descriptor
    GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.pipelineType = PipelineType::Mesh;

    // 从 SPIR-V LocalSize 反射中自动提取 mesh shader 的 threadgroup 大小
    if (meshShaderInfo->threadgroupSizeX > 0)
    {
        pipelineDesc.meshThreadgroupSizeX = meshShaderInfo->threadgroupSizeX;
        pipelineDesc.meshThreadgroupSizeY = meshShaderInfo->threadgroupSizeY;
        pipelineDesc.meshThreadgroupSizeZ = meshShaderInfo->threadgroupSizeZ;
    }

    // Object shader dispatches 3 mesh threadgroups per object threadgroup
    pipelineDesc.maxObjectPayloadMeshlets = 3;

    // color attachment (default RGBA8)
    pipelineDesc.renderTargetCount = 1;
    pipelineDesc.colorAttachmentDescriptors[0].blendingEnabled = false;
    pipelineDesc.colorAttachmentDescriptors[0].writeMask = ColorWriteMaskAll;

    // depth attachment
    pipelineDesc.depthStencilDescriptor.depthCompareFunction = CompareFunctionLessThanOrEqual;
    pipelineDesc.depthStencilDescriptor.depthWriteEnabled = true;

    // create pipeline
    mMeshPipeline = mRenderDevice->CreateGraphicsPipeline(pipelineDesc);
    if (!mMeshPipeline)
    {
        LOG_ERROR("Failed to create mesh pipeline");
        return;
    }

    // attach shaders
    mMeshPipeline->AttachTaskShader(taskShader);
    mMeshPipeline->AttachMeshShader(meshShader);
    mMeshPipeline->AttachFragmentShader(fragmentShader);

    // create uniform buffer (3 mat4 = 192 bytes)
    mUniformBuffer = mRenderDevice->CreateUniformBufferWithSize(sizeof(UniformData));
    if (!mUniformBuffer)
    {
        LOG_ERROR("Failed to create uniform buffer");
        return;
    }

    LOG_INFO("Mesh shader pipeline created successfully");
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

    // draw: 1 object threadgroup -> object shader dispatches 3 mesh threadgroups -> 3 triangles
    renderEncoder->DrawMeshTasks(1, 1, 1);

    renderEncoder->EndEncode();
    commandBuffer->PresentFrameBuffer();
}

void MeshShaderFrameWork::OnEvent(GNXEngine::Event& e)
{
    GNXEngine::AppFrameWork::OnEvent(e);
}
