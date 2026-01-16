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
 * 支持的光源类型：
 * - DirectionalLight：使用级联阴影贴图（CSM）
 * - SpotLight：使用单个透视投影的ShadowMap
 * - PointLight：使用立方体ShadowMap（6个方向）
 *
 * 注意：本类不负责实际的渲染操作，只负责：
 * 1. 创建和管理深度纹理资源
 * 2. 计算光源的视图投影矩阵
 * 3. 提供深度纹理和矩阵供外部使用
 *
 * 实际的渲染调用应该在外部的渲染循环中进行，使用RenderEncoder进行绘制。
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
     * @brief 更新方向光阴影矩阵
     * @param light 方向光源
     * @param camera 摄像机（用于级联计算，可为nullptr）
     */
    void UpdateDirectionalLightShadows(Light* light, Camera* camera);

    /**
     * @brief 更新聚光灯阴影矩阵
     * @param light 聚光源
     */
    void UpdateSpotLightShadows(Light* light);

    /**
     * @brief 更新点光源阴影矩阵
     * @param light 点光源
     */
    void UpdatePointLightShadows(Light* light);

    /**
     * @brief 获取阴影贴图纹理
     * @param light 光源
     * @return 阴影贴图纹理指针
     */
    RCTexturePtr GetShadowMap(Light* light) const;

    /**
     * @brief 获取级联阴影贴图（用于DirectionalLight）
     * @param cascadeIndex 级联索引
     * @return 阴影贴图纹理指针
     */
    RCTexturePtr GetCascadeShadowMap(uint32_t cascadeIndex) const;

    /**
     * @brief 获取立方体阴影贴图（用于PointLight）
     * @param faceIndex 面索引（0-5）
     * @return 阴影贴图纹理指针
     */
    RCTexturePtr GetCubeShadowMap(uint32_t faceIndex) const;

    /**
     * @brief 获取深度预通过纹理
     * @return 深度纹理指针
     */
    RCTexturePtr GetDepthTexture() const;

    /**
     * @brief 获取光源视图矩阵
     * @param light 光源
     * @param cascadeIndex 级联索引（仅适用于DirectionalLight）
     * @return 视图矩阵
     */
    mathutil::Matrix4x4f GetLightViewMatrix(Light* light, uint32_t cascadeIndex = 0) const;

    /**
     * @brief 获取光源投影矩阵
     * @param light 光源
     * @param cascadeIndex 级联索引（仅适用于DirectionalLight）
     * @return 投影矩阵
     */
    mathutil::Matrix4x4f GetLightProjectionMatrix(Light* light, uint32_t cascadeIndex = 0) const;

    /**
     * @brief 获取级联阴影分割距离
     * @param cascadeIndex 级联索引
     * @return 分割距离
     */
    float GetCascadeSplitDistance(uint32_t cascadeIndex) const;

    /**
     * @brief 获取级联数量
     * @return 级联数量
     */
    uint32_t GetCascadeCount() const;

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

    /**
     * @brief 为DirectionalLight计算级联阴影分割
     * @param nearPlane 近裁剪面
     * @param farPlane 远裁剪面
     */
    void CalculateCascadeSplits(float nearPlane, float farPlane);

    /**
     * @brief 为DirectionalLight计算级联视图投影矩阵
     * @param light 光源
     * @param camera 相机（可为nullptr）
     */
    void CalculateCascadeMatrices(Light* light, Camera* camera);

    /**
     * @brief 为SpotLight计算视图投影矩阵
     * @param light 光源
     */
    void CalculateSpotLightMatrices(Light* light);

    /**
     * @brief 为PointLight计算6个方向的视图投影矩阵
     * @param light 光源
     */
    void CalculatePointLightMatrices(Light* light);

    /**
     * @brief 聚合光源的AABB
     * @param scene 场景
     * @return 包围盒
     */
    mathutil::AxisAlignedBoxf GetSceneAABB(Scene* scene) const;

    /**
     * @brief 拟合光源正交投影矩阵到场景
     * @param lightViewMatrix 光源视图矩阵
     * @param nearPlane 近裁剪面
     * @param farPlane 远裁剪面
     * @return 投影矩阵
     */
    mathutil::Matrix4x4f FitLightMatrixToScene(const mathutil::Matrix4x4f& lightViewMatrix,
                                               float nearPlane, float farPlane);

private:
    RenderDevice* mDevice;
    DepthRenderConfig mConfig;

    // 深度纹理
    RCTexturePtr mDepthTexture;
    std::vector<RCTexturePtr> mCascadeShadowMaps;  // 级联阴影贴图
    std::vector<RCTexturePtr> mCubeShadowMaps;      // 立方体阴影贴图（6个面）

    // 视图投影矩阵缓存
    std::vector<mathutil::Matrix4x4f> mCascadeViewMatrices;
    std::vector<mathutil::Matrix4x4f> mCascadeProjMatrices;
    mathutil::Matrix4x4f mSpotLightViewMatrix;
    mathutil::Matrix4x4f mSpotLightProjMatrix;
    std::vector<mathutil::Matrix4x4f> mCubeLightViewMatrices;  // 6个方向的视图矩阵
    mathutil::Matrix4x4f mCubeLightProjMatrix;                 // 立方体投影矩阵

    // 级联分割距离
    std::vector<float> mCascadeSplitDistances;

    // 光源特定的深度纹理缓存
    std::unordered_map<Light*, RCTexturePtr> mLightShadowMaps;
    std::unordered_map<Light*, std::vector<mathutil::Matrix4x4f>> mLightVPs;

    bool mInitialized = false;
};

typedef std::shared_ptr<DepthRenderer> DepthRendererPtr;

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_DEPTH_RENDERER_INCLUDE_H */
