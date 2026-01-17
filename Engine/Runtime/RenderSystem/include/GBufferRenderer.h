//
//  GBufferRenderer.h
//  GNXEngine
//
//  G-Buffer渲染器 - 管理延迟渲染的G-Buffer生成和光照计算
//

#ifndef GNXENGINE_GBUFFER_RENDERER_H
#define GNXENGINE_GBUFFER_RENDERER_H

#include "RSDefine.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"
#include "FrameGraph/GraphNode.h"
#include <memory>
#include <vector>

NS_RENDERSYSTEM_BEGIN

// 前置声明
class Material;
class SceneRenderer;
class FrameGraph;

class RENDERSYSTEM_API GBufferRenderer
{
public:
    GBufferRenderer();
    ~GBufferRenderer();
    
    // 初始化G-Buffer渲染器
    bool Initialize(uint32_t width, uint32_t height);
    
    // 重置大小（窗口大小改变时调用）
    void Resize(uint32_t width, uint32_t height);
    
    // 开始G-Buffer渲染
    void BeginGBufferPass();
    
    // 结束G-Buffer渲染
    void EndGBufferPass();
    
    // 设置当前材质
    void SetMaterial(std::shared_ptr<Material> material);
    
    // 执行延迟光照
    void ExecuteDeferredLighting();
    
    // 获取G-Buffer纹理（用于调试或其他用途）
    RCTexturePtr GetGBufferTexture(uint32_t index) const;
    
    // 获取最终的渲染结果
    RCTexturePtr GetFinalTexture() const;
    
    // 添加到FrameGraph
    void AddToFrameGraph(FrameGraph& frameGraph);
    
    // 获取FrameGraph资源ID（用于调试或FrameGraph集成）
    struct FrameGraphResourceIds
    {
        FrameGraphResource depth;
        FrameGraphResource gBuffer0;
        FrameGraphResource gBuffer1;
        FrameGraphResource gBuffer2;
        FrameGraphResource gBuffer3;  // 可选
        FrameGraphResource finalColor;
    };
    
    const FrameGraphResourceIds& GetFrameGraphResourceIds() const { return mResourceIds; }
    
    // G-Buffer配置
    struct GBufferConfig
    {
        bool enablePositionTexture = false;  // 是否使用独立的位置纹理（false则从深度重建）
        bool useOctahedralNormal = true;   // 使用octahedral法线编码
        uint32_t msaaSampleCount = 1;      // MSAA采样数
    };
    
    void SetConfig(const GBufferConfig& config) { mConfig = config; }
    const GBufferConfig& GetConfig() const { return mConfig; }
    
private:
    // 创建G-Buffer纹理
    void CreateGBufferTextures(uint32_t width, uint32_t height);
    
    // 销毁G-Buffer纹理
    void DestroyGBufferTextures();
    
    // 创建G-Buffer渲染管线
    void CreateGBufferPipeline();
    
    // 创建延迟光照管线
    void CreateLightingPipeline();
    
private:
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    GBufferConfig mConfig;
    
    // G-Buffer纹理（4个render targets）
    std::vector<RCTexturePtr> mGBufferTextures;
    
    // 深度纹理
    RCTexturePtr mDepthTexture = nullptr;
    
    // 最终输出纹理
    RCTexturePtr mFinalTexture = nullptr;
    
    // 当前材质
    std::shared_ptr<Material> mCurrentMaterial = nullptr;
    
    // 渲染管线
    GraphicsPipelinePtr mGBufferPipeline = nullptr;
    GraphicsPipelinePtr mLightingPipeline = nullptr;
    
    // FrameGraph资源ID
    FrameGraphResourceIds mResourceIds;
    
    bool mIsInitialized = false;
};

typedef std::shared_ptr<GBufferRenderer> GBufferRendererPtr;

NS_RENDERSYSTEM_END

#endif // GNXENGINE_GBUFFER_RENDERER_H
