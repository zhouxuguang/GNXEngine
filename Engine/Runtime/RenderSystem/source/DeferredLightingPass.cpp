//
//  DeferredLightingPass.cpp
//  GNXEngine
//
//  延迟光照渲染Pass实现
//

#include "DeferredLightingPass.h"
#include "ShaderAssetLoader.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/RenderPass.h"
#include "Runtime/MathUtil/include/Vector4.h"
#include <cmath>

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

//=============================================================================
// 光源数据结构（用于GPU）
//=============================================================================

// 最大光源数量
constexpr uint32_t MAX_DIRECTIONAL_LIGHTS = 4;
constexpr uint32_t MAX_POINT_LIGHTS = 64;
constexpr uint32_t MAX_SPOT_LIGHTS = 32;

// GPU端的光源数据结构
struct GPUDirectionalLight
{
    Vector4f direction;      // 方向 (xyz), 强度 (w)
    Vector4f color;          // RGB颜色 + 强度
};

struct GPUPointLight
{
    Vector4f position;       // 位置 (xyz), 强度 (w)
    Vector4f color;          // RGB颜色 + 衰减半径
    Vector4f attenuation;    // 常数衰减, 线性衰减, 二次衰减, 范围
};

struct GPUSpotLight
{
    Vector4f position;       // 位置 (xyz), 强度 (w)
    Vector4f direction;      // 方向 (xyz), 内锥角余弦 (w)
    Vector4f color;          // RGB颜色 + 外锥角余弦
    Vector4f attenuation;    // 常数衰减, 线性衰减, 二次衰减, 范围
};

// 光源UBO数据结构
struct LightDataUBO
{
    GPUDirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
    GPUPointLight pointLights[MAX_POINT_LIGHTS];
    GPUSpotLight spotLights[MAX_SPOT_LIGHTS];
    
    Vector4f ambientColor;       // 环境光颜色 + 强度
};

//=============================================================================
// 构造/析构函数
//=============================================================================

DeferredLightingPass::DeferredLightingPass()
{
}

DeferredLightingPass::~DeferredLightingPass()
{
    Shutdown();
}

//=============================================================================
// 初始化/关闭
//=============================================================================

bool DeferredLightingPass::Initialize(const DeferredLightingConfig& config)
{
    if (mInitialized)
    {
        return true;
    }
    
    mConfig = config;
    
    // 创建延迟光照管线
    CreateLightingPipeline();
    
    // 创建光源UBO
    CreateLightUniformBuffers();
    
    mInitialized = true;
    return true;
}

void DeferredLightingPass::Shutdown()
{
    mLightingPipeline = nullptr;
    mLightDataUBO = nullptr;
    mInitialized = false;
}

void DeferredLightingPass::Resize(uint32_t width, uint32_t height)
{
    if (mConfig.width == width && mConfig.height == height)
    {
        return;
    }
    
    mConfig.width = width;
    mConfig.height = height;
}

void DeferredLightingPass::UpdateConfig(const DeferredLightingConfig& config)
{
    bool needResize = (config.width != mConfig.width || config.height != mConfig.height);
    mConfig = config;
    
    if (needResize && mInitialized)
    {
        Resize(config.width, config.height);
    }
}

//=============================================================================
// 创建渲染管线
//=============================================================================

void DeferredLightingPass::CreateLightingPipeline()
{
    // 创建延迟光照着色器
    GraphicsShaderInfo shaderInfo = CreateGraphicsShaderInfo("DeferredLighting");
    
    // 配置深度测试（只读深度）
    shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = true;
    shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthWriteEnabled = false;
    shaderInfo.graphicsPipelineDesc.depthStencilDescriptor.depthCompareFunction = DepthConfig::GetDefaultDepthCompareFunc();
    
    // 创建管线
    mLightingPipeline = RenderCore::GetRenderDevice()->CreateGraphicsPipeline(shaderInfo.graphicsPipelineDesc);
    mLightingPipeline->AttachGraphicsShader(shaderInfo.graphicsShader);
}

//=============================================================================
// 创建光源UBO
//=============================================================================

void DeferredLightingPass::CreateLightUniformBuffers()
{
}

//=============================================================================
// 更新光源数据
//=============================================================================

void DeferredLightingPass::UpdateLightData(const DeferredLightingParams& params)
{
}

//=============================================================================
// 添加到FrameGraph
//=============================================================================

DeferredLightingOutput DeferredLightingPass::AddToFrameGraph(
    const std::string& passName,
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    const DeferredLightingParams& params)
{
    // 定义Pass数据结构
    struct LightingPassData
    {
        DeferredLightingOutput output;
        
        // 输入纹理
        FrameGraphResource gBufferA;
        FrameGraphResource gBufferB;
        FrameGraphResource gBufferC;
        FrameGraphResource depthTexture;
        
        // Uniform Buffers
        UniformBufferPtr cameraUBO;
        UniformBufferPtr lightUBO;
        
        // IBL资源
        bool enableIBL;
        RCTexturePtr irradianceMap;
        RCTexturePtr prefilteredMap;
        RCTexturePtr brdfLUT;
    };
    
    auto& passData = frameGraph.AddPass<LightingPassData>(
        passName,
        [=](FrameGraph::Builder& builder, LightingPassData& data)
        {
            // 创建输出纹理（HDR格式）
            FrameGraphTexture::Desc outputDesc;
            outputDesc.extent = RenderCore::Rect2D{0, 0, (int)mConfig.width, (int)mConfig.height};
            outputDesc.depth = 1;
            outputDesc.format = RenderCore::kTexFormatRGBA16Float;
            data.output.lightingResult = builder.Create<FrameGraphTexture>("LightingResult", outputDesc);
            builder.Write(data.output.lightingResult, (uint32_t)RenderCore::ResourceAccessType::ColorAttachment);
            
            // 读取G-Buffer纹理
            data.gBufferA = builder.Read(params.gBufferA, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            data.gBufferB = builder.Read(params.gBufferB, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            data.gBufferC = builder.Read(params.gBufferC, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            data.depthTexture = builder.Read(params.depthTexture, (uint32_t)RenderCore::ResourceAccessType::ShaderRead);
            
            // 保存Uniform Buffers
            data.cameraUBO = params.cameraUBO;
            data.lightUBO = mLightDataUBO;
            
            // 保存IBL参数
            data.enableIBL = params.enableIBL;
            data.irradianceMap = params.irradianceMap;
            data.prefilteredMap = params.prefilteredMap;
            data.brdfLUT = params.brdfLUT;
        },
        [=](const LightingPassData& data, FrameGraphPassResources& resources, void* context)
        {
            // 更新光源数据
            UpdateLightData(params);
            
            // 获取纹理资源
            FrameGraphTexture& gBufferA = resources.Get<FrameGraphTexture>(data.gBufferA);
            FrameGraphTexture& gBufferB = resources.Get<FrameGraphTexture>(data.gBufferB);
            FrameGraphTexture& gBufferC = resources.Get<FrameGraphTexture>(data.gBufferC);
            FrameGraphTexture& depthTexture = resources.Get<FrameGraphTexture>(data.depthTexture);
            FrameGraphTexture& outputTexture = resources.Get<FrameGraphTexture>(data.output.lightingResult);
            
            float debugColor[4] = {1.0f, 1.0f, 0.0f, 1.0f};
            SCOPED_DEBUGMARKER_EVENT(commandBuffer, resources.GetPassName().c_str(), debugColor);
            
            // 创建RenderPass
            RenderPass renderPass;
            renderPass.renderRegion = Rect2D(0, 0, (int)mConfig.width, (int)mConfig.height);
            
            // 颜色附件
            auto colorAttachment = std::make_shared<RenderPassColorAttachment>();
            colorAttachment->texture = outputTexture.texture;
            colorAttachment->clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
            colorAttachment->loadOp = ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment->storeOp = ATTACHMENT_STORE_OP_STORE;
            renderPass.colorAttachments.push_back(colorAttachment);
            
            // 深度附件（从G-Buffer Pass读取，只读）
            auto depthAttachment = std::make_shared<RenderPassDepthAttachment>();
            depthAttachment->texture = depthTexture.texture;
            depthAttachment->loadOp = ATTACHMENT_LOAD_OP_LOAD;
            depthAttachment->storeOp = ATTACHMENT_STORE_OP_DONT_CARE;
            renderPass.depthAttachment = depthAttachment;
            
            // 创建RenderEncoder
            RenderEncoderPtr renderEncoder = commandBuffer->CreateRenderEncoder(renderPass);
            
            // 绑定渲染管线
            renderEncoder->SetGraphicsPipeline(mLightingPipeline);
            
            // 绑定相机UBO
            if (data.cameraUBO)
            {
                renderEncoder->SetVertexUniformBuffer(data.cameraUBO, 0);
                renderEncoder->SetFragmentUniformBuffer(data.cameraUBO, 0);
            }
            
            // 绑定光源UBO
            if (data.lightUBO)
            {
                renderEncoder->SetFragmentUniformBuffer(data.lightUBO, 1);
            }
            
            renderEncoder->EndEncode();
        }
    );
    
    return passData.output;
}

NS_RENDERSYSTEM_END
