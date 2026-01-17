//
//  DepthRenderer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2025/1/11.
//  深度渲染工具类 - 用于生成ShadowMap和Depth Pre-Pass
//

#ifndef GNX_ENGINE_DEPTH_RENDERER_INCLUDE_H
#define GNX_ENGINE_DEPTH_RENDERER_INCLUDE_H

#include "RSDefine.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include "Runtime/MathUtil/include/AABB.h"
#include "Camera.h"
#include "Light.h"
#include "Scene.h"
#include <memory>
#include <vector>
#include <limits>
#include <unordered_map>

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

/**
 * @brief 深度渲染器配置参数
 */
struct DepthRenderConfig
{
    uint32_t width = 1024;              // 深度纹理宽度
    uint32_t height = 1024;             // 深度纹理高度
    uint32_t cascadeCount = 4;          // 级联阴影贴图数量（仅适用于DirectionalLight）
    float nearPlane = 0.1f;             // 近裁剪面距离
    float farPlane = 100.0f;            // 远裁剪面距离
    float shadowBias = 0.005f;          // 阴影偏移（防止 acne）
    TextureFormat depthFormat = kTexFormatDepth32Float;  // 深度格式

    // 级联阴影分割参数（仅适用于DirectionalLight）
    float cascadeSplitLambda = 0.5f;     // 级联分割系数（0=对数分割，1=均匀分割）
};

/**
 * @brief 深度渲染器类
 *
 * 负责生成各种深度图像：
 * - ShadowMap：用于阴影渲染
 * - Depth Pre-Pass：用于提前剔除不可见像素，提升性能
 *
 */
class RENDERSYSTEM_API DepthRenderer
{
public:
    DepthRenderer(RenderDevice* device);
    ~DepthRenderer();

    /**
     * @brief 初始化深度渲染器
     * @param config 配置参数
     */
    bool Initialize(const DepthRenderConfig& config);

    /**
     * @brief 关闭深度渲染器，释放资源
     */
    void Shutdown();

    /**
     * @brief 获取深度预通过纹理
     * @return 深度纹理指针
     */
    RCTexturePtr GetDepthTexture() const;

    /**
     * @brief 更新配置参数
     * @param config 新的配置参数
     */
    void UpdateConfig(const DepthRenderConfig& config);

    /**
     * @brief 获取当前配置
     * @return 配置参数
     */
    const DepthRenderConfig& GetConfig() const;

    /**
     * @brief 设置阴影偏移
     * @param bias 偏移值
     */
    void SetShadowBias(float bias) { mConfig.shadowBias = bias; }

    /**
     * @brief 获取阴影偏移
     * @return 偏移值
     */
    float GetShadowBias() const { return mConfig.shadowBias; }

    /**
     * @brief 是否已初始化
     * @return 是否已初始化
     */
    bool IsInitialized() const { return mInitialized; }

private:
    /**
     * @brief 创建深度纹理
     * @param width 宽度
     * @param height 高度
     * @param format 格式
     * @return 深度纹理指针
     */
    RCTexturePtr CreateDepthTexture(uint32_t width, uint32_t height, TextureFormat format);

private:
    RenderDevice* mDevice;
    DepthRenderConfig mConfig;

    // 深度纹理
    RCTexturePtr mDepthTexture;
    bool mInitialized = false;
};

using DepthRendererPtr = std::shared_ptr<DepthRenderer>;
using DepthRendererUniPtr = std::unique_ptr<DepthRenderer>;

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_DEPTH_RENDERER_INCLUDE_H */
