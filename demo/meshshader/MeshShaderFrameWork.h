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

    RenderCore::RenderDevicePtr mRenderDevice = nullptr;
    RenderCore::GraphicsPipelinePtr mMeshPipeline = nullptr;
    RenderCore::UniformBufferPtr mUniformBuffer = nullptr;

    uint32_t mWidth = 1280;
    uint32_t mHeight = 720;
};

#endif /* MeshShaderFrameWork_h */
