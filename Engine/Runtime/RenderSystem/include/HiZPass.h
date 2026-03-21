//
//  HiZPass.h
//  GNXEngine
//
//  Hi-Z (Hierarchical Z-Buffer) 生成Pass
//  用于SSR加速、遮挡剔除等
//
//  Created by zhouxuguang
//

#ifndef GNX_ENGINE_HIZ_PASS_INCLUDE_H
#define GNX_ENGINE_HIZ_PASS_INCLUDE_H

#include "RSDefine.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/FrameGraphTexture.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include "Runtime/RenderCore/include/TextureSampler.h"

NS_RENDERSYSTEM_BEGIN

// Hi-Z最大层级数（根据屏幕分辨率自动计算）
static const uint32_t kMaxHiZLevels = 16;

/**
 * @brief Hi-Z Pass的输入参数
 */
struct HiZParams
{
    uint32_t width;                         // 屏幕宽度
    uint32_t height;                        // 屏幕高度
    FrameGraphResource depthTexture;        // 输入：深度缓冲
    
    // 可选参数
    bool useReverseZ = false;               // 是否使用Reverse-Z
    bool enableDebug = false;               // 是否启用调试输出
};

/**
 * @brief Hi-Z Pass的输出结果
 */
struct HiZOutput
{
    FrameGraphResource hiZTexture;          // 输出：Hi-Z纹理（完整Mip链）
    uint32_t numLevels;                     // Hi-Z层级数量
};

/**
 * @brief Hi-Z生成Pass
 * 
 * 功能：
 * 1. 从深度缓冲生成层级深度金字塔
 * 2. 每一层是上一层的2x2降采样，取最大（或最小）深度值
 * 3. 用于SSR光线步进加速、遮挡剔除等
 * 
 * 算法原理：
 * - Level 0: 原始深度缓冲（1920x1080）
 * - Level 1: 960x540（每2x2像素取最大值）
 * - Level 2: 480x270
 * - ...
 * - Level N: 2x2 或更小
 * 
 * 对于Reverse-Z：取最小值（近处=1，远处=0）
 * 对于传统Z：取最大值（近处=0，远处=1）
 */
class RENDERSYSTEM_API HiZPass
{
public:
    HiZPass();
    ~HiZPass();
    
    /**
     * @brief 初始化Hi-Z Pass
     * @param useReverseZ 是否使用Reverse-Z深度模式
     * @return 是否初始化成功
     */
    bool Initialize(bool useReverseZ = false);
    
    /**
     * @brief 添加Hi-Z生成Pass到FrameGraph
     * 
     * @param passName Pass名称
     * @param frameGraph 帧图
     * @param commandBuffer 命令缓冲（用于创建Compute Encoder）
     * @param params 渲染参数
     * @return Hi-Z输出结果
     */
    HiZOutput AddToFrameGraph(
        const std::string& passName,
        FrameGraph& frameGraph,
        CommandBufferPtr commandBuffer,
        const HiZParams& params);
    
    /**
     * @brief 获取指定层级的Hi-Z纹理视图
     * @param level 层级索引（0 = 最高分辨率）
     * @return 纹理视图指针，如果层级无效则返回nullptr
     */
    RenderCore::RCTexturePtr GetHiZLevel(uint32_t level) const;
    
    /**
     * @brief 获取Hi-Z层级数量
     */
    uint32_t GetNumLevels() const { return mHiZLevels; }
    
    /**
     * @brief 获取完整的Hi-Z纹理
     */
    RenderCore::RCTexturePtr GetHiZTexture() const { return mHiZTexture; }
    
    /**
     * @brief 获取Hi-Z采样器（Max Reduction）
     */
    RenderCore::TextureSamplerPtr GetHiZSampler() const { return mHiZSampler; }
    
    /**
     * @brief 是否已初始化
     */
    bool IsInitialized() const { return mInitialized; }
    
    /**
     * @brief 清理GPU资源
     */
    void FreeGPUResources();

private:
    /**
     * @brief 创建Hi-Z生成Compute Pipeline
     */
    void CreateHiZPipeline();
    
    /**
     * @brief 创建Hi-Z纹理和视图
     * @param width 基础宽度
     * @param height 基础高度
     */
    void CreateHiZTexture(uint32_t width, uint32_t height);
    
    /**
     * @brief 计算Hi-Z层级数量
     * @param width 宽度
     * @param height 高度
     * @return 层级数量
     */
    uint32_t CalculateNumLevels(uint32_t width, uint32_t height) const;

private:
    bool mInitialized = false;
    bool mUseReverseZ = false;
    
    // Compute Pipeline
    RenderCore::ComputePipelinePtr mHiZPipeline;
    RenderCore::UniformBufferPtr mHiZParas;
    
    // Hi-Z纹理资源
    RenderCore::RCTexturePtr mHiZTexture;                    // 完整的Hi-Z纹理（带Mip链）
    RenderCore::RCTexturePtr mHiZViews[kMaxHiZLevels];       // 每一层的纹理视图
    RenderCore::TextureSamplerPtr mHiZSampler;               // Max Reduction采样器
    
    // Hi-Z配置
    uint32_t mHiZLevels = 0;
    uint32_t mCurrentWidth = 0;
    uint32_t mCurrentHeight = 0;
    
    // CommandBuffer引用（用于创建Compute Encoder）
    CommandBufferPtr mCommandBuffer;
};

typedef std::shared_ptr<HiZPass> HiZPassPtr;

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_HIZ_PASS_INCLUDE_H */
