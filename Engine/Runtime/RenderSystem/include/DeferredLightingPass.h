//
//  DeferredLightingPass.h
//  GNXEngine
//
//  延迟光照渲染Pass - 读取G-Buffer计算场景光照
//

#ifndef GNXENGINE_DEFERRED_LIGHTING_PASS_H
#define GNXENGINE_DEFERRED_LIGHTING_PASS_H

#include "RSDefine.h"
#include "Light.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/FrameGraphTexture.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include "Runtime/MathUtil/include/SimdMath.h"
#include <memory>
#include <vector>

NS_RENDERSYSTEM_BEGIN

// 前置声明
class GBufferData;

/**
 * @brief 延迟光照渲染参数
 * 
 * 包含G-Buffer资源、光源列表、相机信息等
 */
struct DeferredLightingParams
{
    uint32_t width;
    uint32_t height;
    // G-Buffer 纹理资源
    FrameGraphResource gSceneColor;
    FrameGraphResource gBufferA;
    FrameGraphResource gBufferB;
    FrameGraphResource gBufferC;
    FrameGraphResource gBufferD;
    FrameGraphResource depthTexture;   
    
    FrameGraphResource ssaoTexture = -1;
    
    // 光源数据
    std::vector<DirectionLight*> directionalLights;
    std::vector<PointLight*> pointLights;
    std::vector<SpotLight*> spotLights;
    
    // 相机 Uniform Buffer
    UniformBufferPtr cameraUBO = nullptr;
    
    // 光源 Uniform Buffer（可选，用于大量光源时）
    UniformBufferPtr lightUBO = nullptr;
    
    // 环境光参数
    mathutil::Vector3f ambientColor = mathutil::Vector3f(0.03f, 0.03f, 0.03f);
    float ambientIntensity = 1.0f;
    
    // 是否启用IBL
    bool enableIBL = false;
    RCTexturePtr irradianceMap = nullptr;
    RCTexturePtr prefilteredMap = nullptr;
    RCTexturePtr brdfLUT = nullptr;

    // ========== 阴影（PCSS） ==========
    // ShadowMap 深度纹理（FrameGraph 资源，-1 表示无阴影）
    FrameGraphResource shadowMap = -1;

    // 阴影数据（光源视图/投影矩阵 + 参数），供 shader 端 PCSS 采样
    mathutil::Matrix4x4f shadowLightView;                  // 光源视图矩阵
    mathutil::Matrix4x4f shadowLightProj;                  // 光源投影矩阵（Reverse-Z 正交）
    mathutil::simd_float4 shadowMapSize = mathutil::make_simd_float4(0, 0, 0, 0); // (w, h, 1/w, 1/h)
    mathutil::simd_float4 shadowParams = mathutil::make_simd_float4(0.0005f, 0.02f, 0.02f, 3.0f); // (bias, normalBias, lightSize, filterRadius)
    bool enableShadow = false;
};

/**
 * @brief 延迟光照输出结果
 */
struct DeferredLightingOutput
{
    FrameGraphResource lightingResult;  // 光照计算结果（HDR）
};

/**
 * @brief 延迟光照Pass配置
 */
struct DeferredLightingConfig
{
    bool enableSSAO = false;            // 是否启用SSAO
    bool enableSSR = false;             // 是否启用SSR
    bool enableTiledLighting = false;   // 是否使用Tile-Based光照
};

/**
 * @brief 延迟光照渲染Pass
 * 
 * 负责从G-Buffer读取几何信息并计算场景光照：
 * - 支持方向光、点光源、聚光灯
 * - 支持PBR材质光照模型
 * - 可选Tile-Based光照优化
 */
class RENDERSYSTEM_API DeferredLightingPass
{
public:
    DeferredLightingPass();
    ~DeferredLightingPass();
    
    /**
     * @brief 初始化延迟光照Pass
     * @param config 配置参数
     * @return 是否初始化成功
     */
    bool Initialize(const DeferredLightingConfig& config);
    
    /**
     * @brief 添加延迟光照Pass到FrameGraph
     * @param passName Pass名称
     * @param frameGraph 帧图
     * @param commandBuffer 命令缓冲
     * @param params 渲染参数
     * @return 光照输出结果
     */
    DeferredLightingOutput AddToFrameGraph(
        const std::string& passName,
        FrameGraph& frameGraph,
        CommandBufferPtr commandBuffer,
        const DeferredLightingParams& params);
    
    /**
     * @brief 获取当前配置
     */
    const DeferredLightingConfig& GetConfig() const { return mConfig; }
    
    /**
     * @brief 是否已初始化
     */
    bool IsInitialized() const { return mInitialized; }

private:
    /**
     * @brief 创建延迟光照渲染管线
     */
    void CreateLightingPipeline();
    
    /**
     * @brief 创建光源Uniform Buffer
     */
    void CreateLightUniformBuffers();

    /**
     * @brief 创建阴影Uniform Buffer
     */
    void CreateShadowUniformBuffer();

    /**
     * @brief 更新阴影数据到UBO
     */
    void UpdateShadowData(const DeferredLightingParams& params);

    /**
     * @brief 更新光源数据到UBO
     */
    void UpdateLightData(const DeferredLightingParams& params);

private:
    DeferredLightingConfig mConfig;
    
    // 延迟光照管线
    GraphicsPipelinePtr mLightingPipeline = nullptr;
    
    // G-Buffer采样器
    TextureSamplerPtr mGBufferSampler = nullptr;

    // IBL cubemap 采样器（带 mip trilinear，用于预过滤环境贴图的 roughness LOD 采样）
    TextureSamplerPtr mIBLCubeSampler = nullptr;

    // ShadowMap 采样器（PCSS 需要非比较采样，手动做深度比较）
    TextureSamplerPtr mShadowSampler = nullptr;
    
    // 光源数据UBO
    UniformBufferPtr mLightDataUBO = nullptr;

    // 阴影数据UBO
    UniformBufferPtr mShadowUBO = nullptr;
    
    bool mInitialized = false;
    
    uint32_t mWidth = 1;
    uint32_t mHeight = 1;
};

typedef std::shared_ptr<DeferredLightingPass> DeferredLightingPassPtr;

NS_RENDERSYSTEM_END

#endif // GNXENGINE_DEFERRED_LIGHTING_PASS_H
