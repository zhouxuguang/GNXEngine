//
//  GBufferRenderer.cpp
//  GNXEngine
//
//  G-Buffer渲染器实现
//

#include "GBufferRenderer.h"
#include "Material.h"
#include "SceneRenderer.h"
#include "FrameGraph/FrameGraph.h"
#include "ShaderAssetLoader.h"
#include "Runtime/RenderCore/include/RenderDevice.h"

NS_RENDERSYSTEM_BEGIN

GBufferRenderer::GBufferRenderer()
{
}

GBufferRenderer::~GBufferRenderer()
{
    DestroyGBufferTextures();
}

bool GBufferRenderer::Initialize(uint32_t width, uint32_t height)
{
    if (mIsInitialized)
    {
        return true;
    }
    
    mWidth = width;
    mHeight = height;
    
    // 创建G-Buffer纹理
    CreateGBufferTextures(width, height);
    
    // 创建渲染管线
    CreateGBufferPipeline();
    CreateLightingPipeline();
    
    mIsInitialized = true;
    return true;
}

void GBufferRenderer::Resize(uint32_t width, uint32_t height)
{
    if (mWidth == width && mHeight == height)
    {
        return;
    }
    
    mWidth = width;
    mHeight = height;
    
    // 重新创建G-Buffer纹理
    DestroyGBufferTextures();
    CreateGBufferTextures(width, height);
}

void GBufferRenderer::BeginGBufferPass()
{
    // TODO: 设置渲染目标到G-Buffer纹理
    // 这里需要调用RenderDevice的API来设置多个渲染目标
}

void GBufferRenderer::EndGBufferPass()
{
    // TODO: 恢复默认渲染目标
}

void GBufferRenderer::SetMaterial(std::shared_ptr<Material> material)
{
    mCurrentMaterial = material;
    
    if (material && material->GetGBufferPSO())
    {
        // 使用材质的G-Buffer PSO
        // TODO: 设置当前渲染管线
    }
}

void GBufferRenderer::ExecuteDeferredLighting()
{
    // TODO: 执行延迟光照Pass
    // 1. 绑定G-Buffer纹理
    // 2. 绘制全屏四边形
    // 3. 输出到最终纹理
}

RCTexturePtr GBufferRenderer::GetGBufferTexture(uint32_t index) const
{
    if (index < mGBufferTextures.size())
    {
        return mGBufferTextures[index];
    }
    return nullptr;
}

RCTexturePtr GBufferRenderer::GetFinalTexture() const
{
    return mFinalTexture;
}

void GBufferRenderer::AddToFrameGraph(FrameGraph& frameGraph)
{
    // TODO: 将G-Buffer Pass添加到FrameGraph
    // 示例代码（需要根据实际FrameGraph API调整）:
    /*
    struct GBufferData
    {
        FrameGraphResource depth;
        std::vector<FrameGraphResource> gBufferOutputs;
    };
    
    frameGraph.AddPass<GBufferData>(
        "GBufferPass",
        [&](FrameGraph::Builder& builder, GBufferData& data) {
            // 创建G-Buffer资源
            for (int i = 0; i < 4; i++)
            {
                auto desc = TextureDescriptor::Create2D(
                    mWidth, mHeight,
                    GetGBufferFormat(i),
                    TextureUsageRenderTarget);
                data.gBufferOutputs.push_back(builder.Create<GBufferTexture>(
                    "GBuffer" + std::to_string(i), desc));
            }
        },
        [](const GBufferData& data, FrameGraphPassResources& resources, void* context) {
            // 执行G-Buffer渲染
        }
    );
    */
}

void GBufferRenderer::CreateGBufferTextures(uint32_t width, uint32_t height)
{
    auto device = GetRenderDevice();
    
    // 创建4个G-Buffer纹理
    // RT0: Albedo + Opacity (RGBA8_UNORM)
    // RT1: Normal + Roughness (RGBA16_FLOAT 或 RGBA8_UNORM with encoding)
    // RT2: Metallic + AO + Emissive (RGBA8_UNORM)
    // RT3: Position (RGBA16_FLOAT 或省略，从深度重建)
    
    // TODO: 根据具体RenderDevice API创建纹理
    // 示例伪代码：
    /*
    // RT0: Albedo
    TextureDescriptor albedoDesc;
    albedoDesc.width = width;
    albedoDesc.height = height;
    albedoDesc.format = TextureFormat::RGBA8_UNORM;
    albedoDesc.usage = TextureUsageRenderTarget | TextureUsageShaderRead;
    mGBufferTextures[0] = device->CreateTexture(albedoDesc);
    
    // RT1: Normal + Roughness
    TextureDescriptor normalDesc;
    normalDesc.width = width;
    normalDesc.height = height;
    normalDesc.format = mConfig.useOctahedralNormal ? 
                         TextureFormat::RGBA8_UNORM : TextureFormat::RGBA16_FLOAT;
    normalDesc.usage = TextureUsageRenderTarget | TextureUsageShaderRead;
    mGBufferTextures[1] = device->CreateTexture(normalDesc);
    
    // RT2: Metallic + AO + Emissive
    TextureDescriptor materialDesc;
    materialDesc.width = width;
    materialDesc.height = height;
    materialDesc.format = TextureFormat::RGBA8_UNORM;
    materialDesc.usage = TextureUsageRenderTarget | TextureUsageShaderRead;
    mGBufferTextures[2] = device->CreateTexture(materialDesc);
    
    // RT3: Position (可选)
    if (mConfig.enablePositionTexture)
    {
        TextureDescriptor positionDesc;
        positionDesc.width = width;
        positionDesc.height = height;
        positionDesc.format = TextureFormat::RGBA16_FLOAT;
        positionDesc.usage = TextureUsageRenderTarget | TextureUsageShaderRead;
        mGBufferTextures[3] = device->CreateTexture(positionDesc);
    }
    
    // 深度纹理
    TextureDescriptor depthDesc;
    depthDesc.width = width;
    depthDesc.height = height;
    depthDesc.format = TextureFormat::D24_UNORM_S8_UINT;
    depthDesc.usage = TextureUsageDepthStencil;
    mDepthTexture = device->CreateTexture(depthDesc);
    
    // 最终输出纹理
    TextureDescriptor finalDesc;
    finalDesc.width = width;
    finalDesc.height = height;
    finalDesc.format = TextureFormat::RGBA16_FLOAT;  // HDR
    finalDesc.usage = TextureUsageRenderTarget | TextureUsageShaderRead;
    mFinalTexture = device->CreateTexture(finalDesc);
    */
}

void GBufferRenderer::DestroyGBufferTextures()
{
    mGBufferTextures.clear();
    mDepthTexture = nullptr;
    mFinalTexture = nullptr;
}

void GBufferRenderer::CreateGBufferPipeline()
{
    // 加载G-Buffer shader
    // 这里可以加载不同材质类型的G-Buffer shader变体
    
    // 示例：
    /*
    ShaderAssetString shaderAsset = LoadShaderAsset("GBufferPBR");
    GraphicsPipelineDescriptor pipelineDesc;
    
    // 设置多渲染目标
    pipelineDesc.colorAttachmentCount = mConfig.enablePositionTexture ? 4 : 3;
    
    // 设置深度测试
    pipelineDesc.depthStencilDescriptor.depthWriteEnabled = true;
    pipelineDesc.depthStencilDescriptor.depthCompareFunction = DepthConfig::GetDefaultDepthCompareFunc();
    
    // 创建PSO
    mGBufferPipeline = GetRenderDevice()->CreateGraphicsPipeline(pipelineDesc);
    */
}

void GBufferRenderer::CreateLightingPipeline()
{
    // 加载延迟光照shader
    /*
    ShaderAssetString shaderAsset = LoadShaderAsset("DeferredLighting");
    GraphicsPipelineDescriptor pipelineDesc;
    
    // 单个渲染目标
    pipelineDesc.colorAttachmentCount = 1;
    
    // 不需要深度测试（全屏四边形）
    pipelineDesc.depthStencilDescriptor.depthTestEnabled = false;
    
    mLightingPipeline = GetRenderDevice()->CreateGraphicsPipeline(pipelineDesc);
    */
}

NS_RENDERSYSTEM_END
