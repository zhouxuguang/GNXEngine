//
//  MeshShaderFrameWork.h
//  meshshader
//
//  Mesh Shader Demo - Displays 3 overlapping colored triangles
//  using task (amplification) + mesh shaders.
//

#ifndef MeshShaderFrameWork_h
#define MeshShaderFrameWork_h

#include "Runtime/GNXEngine/include/AppFrameWork.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"
#include "Runtime/RenderCore/include/UniformBuffer.h"
#include "Runtime/RenderCore/include/RCBuffer.h"

// Must match VertexData struct in MeshShaderDemo.shader (float4 position + float4 color)
struct alignas(16) SSBOVertexData
{
    float position[4];  // x, y, z, w
    float color[4];     // r, g, b, a
};

class MeshShaderFrameWork : public GNXEngine::AppFrameWork
{
public:
    MeshShaderFrameWork(const GNXEngine::WindowProps& props);

    virtual void Initlize() override;
    virtual void Resize(uint32_t width, uint32_t height) override;
    virtual void RenderFrame() override;
    virtual void OnEvent(GNXEngine::Event& e) override;

private:
    void CreatePipeline();
    void CreateVertexSSBO();

    RenderCore::RenderDevicePtr mRenderDevice = nullptr;
    RenderCore::GraphicsPipelinePtr mMeshPipeline = nullptr;
    RenderCore::UniformBufferPtr mUniformBuffer = nullptr;

    // SSBO: vertex data uploaded to GPU for mesh shader to read
    RenderCore::RCBufferPtr mVertexSSBO = nullptr;

    uint32_t mWidth = 1280;
    uint32_t mHeight = 720;
};

#endif /* MeshShaderFrameWork_h */
