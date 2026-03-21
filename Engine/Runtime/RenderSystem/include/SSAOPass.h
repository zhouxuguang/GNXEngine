//
//  SSAOPass.h
//  GNXEngine
//
//  屏幕空间环境光遮蔽(SSAO)渲染Pass
//

#ifndef GNXENGINE_SSAO_PASS_H
#define GNXENGINE_SSAO_PASS_H

#include "RSDefine.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/FrameGraphTexture.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include <memory>
#include <vector>

NS_RENDERSYSTEM_BEGIN

/**
 * @brief SSAO渲染参数
 */
struct SSAOParams
{
    uint32_t width;
    uint32_t height;
    
    // G-Buffer 纹理资源
    FrameGraphResource gBufferA;        // Normal
    FrameGraphResource depthTexture;    // 深度纹理
    
    // 相机 Uniform Buffer
    UniformBufferPtr cameraUBO = nullptr;
    
    // SSAO 参数
    float radius = 0.5f;                // 采样半径
    float bias = 0.025f;                // 偏移量，防止自遮挡
    int kernelSize = 64;                // 采样核心大小
    
    // 是否启用模糊
    bool enableBlur = true;
    int blurRadius = 2;                 // 模糊半径
};

/**
 * @brief SSAO输出结果
 */
struct SSAOOutput
{
    FrameGraphResource ssaoResult;      // SSAO结果纹理
};

/**
 * @brief SSAO Pass配置
 */
struct SSAOConfig
{
    int kernelSize = 64;                // 采样核心数量
    int noiseSize = 4;                  // 噪声纹理大小 (4x4)
    float radius = 0.5f;                // 采样半径
    float bias = 0.025f;                // 深度偏移
    bool enableBlur = true;             // 是否启用模糊
    int blurRadius = 2;                 // 模糊半径
};

/**
 * @brief 屏幕空间环境光遮蔽(SSAO)渲染Pass
 * 
 * 实现基于屏幕空间的环境光遮蔽效果：
 * - 从G-Buffer法线和深度重建视图空间位置
 * - 使用半球采样核心进行遮挡计算
 * - 支持可配置的模糊后处理
 */
class RENDERSYSTEM_API SSAOPass
{
public:
    SSAOPass();
    ~SSAOPass();
    
    /**
     * @brief 初始化SSAO Pass
     * @param config 配置参数
     * @return 是否初始化成功
     */
    bool Initialize(const SSAOConfig& config);
    
    /**
     * @brief 添加SSAO Pass到FrameGraph
     * @param passName Pass名称
     * @param frameGraph 帧图
     * @param commandBuffer 命令缓冲
     * @param params 渲染参数
     * @return SSAO输出结果
     */
    SSAOOutput AddToFrameGraph(
        const std::string& passName,
        FrameGraph& frameGraph,
        CommandBufferPtr commandBuffer,
        const SSAOParams& params);
    
    /**
     * @brief 是否已初始化
     */
    bool IsInitialized() const { return mInitialized; }

private:
    /**
     * @brief 生成SSAO采样核心
     * 在半球内生成均匀分布的采样点，中心密度更高
     */
    void GenerateSampleKernel();
    
    /**
     * @brief 生成随机旋转纹理
     * 创建4x4的随机旋转纹理用于消除带状伪影
     */
    void GenerateRandomRotationTexture();
    
    /**
     * @brief 创建SSAO渲染管线
     */
    void CreateSSAOPipeline();
    
    /**
     * @brief 创建模糊渲染管线
     */
    void CreateBlurPipeline();
    
    /**
     * @brief 创建SSAO参数UBO
     */
    void CreateSSAOUniformBuffers();

private:
    SSAOConfig mConfig;
    
    // SSAO管线
    GraphicsPipelinePtr mSSAOPipeline = nullptr;
    
    // 模糊管线
    GraphicsPipelinePtr mBlurPipeline = nullptr;
    
    // 纹理采样器
    TextureSamplerPtr mGBufferSampler = nullptr;
    TextureSamplerPtr mNoiseSampler = nullptr;
    
    // SSAO采样核心
    std::vector<mathutil::Vector3f> mSampleKernel;
    
    // 随机旋转纹理
    RCTexturePtr mNoiseTexture = nullptr;
    
    // SSAO参数UBO
    UniformBufferPtr mSSAOParamsUBO = nullptr;
    
    bool mInitialized = false;
    
    uint32_t mWidth = 1;
    uint32_t mHeight = 1;
};

typedef std::shared_ptr<SSAOPass> SSAOPassPtr;

NS_RENDERSYSTEM_END

#endif // GNXENGINE_SSAO_PASS_H
