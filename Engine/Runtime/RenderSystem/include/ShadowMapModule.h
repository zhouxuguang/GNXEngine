//
//  ShadowMapModule.h
//  GNXEngine
//
//  阴影模块 - 负责从主方向光视角生成 ShadowMap 深度纹理
//
//  设计要点：
//  - 输入：作为投射体（caster）的网格列表 + 主方向光 + 主相机
//  - 根据方向光方向构造"光源相机"（正交投影，覆盖主相机可见场景范围）
//  - 复用 DepthRenderer 把所有投射体在光源相机下绘制为深度图
//  - 输出：ShadowMap 的 FrameGraphResource + 光照阶段使用的 cbShadow 数据 UBO
//

#ifndef GNXENGINE_SHADOW_MAP_MODULE_H
#define GNXENGINE_SHADOW_MAP_MODULE_H

#include "RSDefine.h"
#include "DepthRenderer.h"
#include "Light.h"
#include "RenderParameter.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include <memory>
#include <vector>

NS_RENDERSYSTEM_BEGIN

// 前置声明
class Camera;
class DirectionLight;
class FrameGraph;

/**
 * @brief 阴影渲染参数
 * 包含投射体网格、主相机（确定阴影覆盖范围）和方向光
 */
struct ShadowMapRenderParams
{
    uint32_t width = 2048;                    // ShadowMap 宽度
    uint32_t height = 2048;                   // ShadowMap 高度

    // 投射体（caster）：所有会投射阴影的网格
    std::vector<DepthMeshItem> staticMeshes;
    std::vector<DepthSkinnedMeshItem> skinnedMeshes;

    // 主相机（用于计算正交投影覆盖范围）
    Camera* camera = nullptr;

    // 主方向光
    DirectionLight* light = nullptr;

    // 蒙皮网格骨骼矩阵 UBO（可选）
    UniformBufferPtr skinnedMatrixUBO = nullptr;
};

/**
 * @brief 阴影模块输出
 */
struct ShadowMapModuleOutput
{
    FrameGraphResource shadowMap = -1;        // ShadowMap 深度纹理（FrameGraph 资源）

    // 光源视图/投影矩阵（用于在延迟光照阶段把世界坐标变换到光源空间，
    // 与阴影深度 pass 使用的矩阵完全一致，保证深度比较一致）
    mathutil::Matrix4x4f lightView;
    mathutil::Matrix4x4f lightProj;

    // 阴影参数（供 shader 使用）
    mathutil::simd_float4 shadowMapSize;      // (width, height, 1/width, 1/height)
    float depthBias = 0.0f;                    // 深度偏移（缓解阴影痤疮）
    float normalBias = 0.0f;                   // 法线偏移
    float lightSize = 0.01f;                   // PCSS：光源区域大小（决定软阴影程度）
    float filterRadius = 1.0f;                 // PCSS：基础 PCF 半径（纹素）
    bool valid = false;
};

/**
 * @brief 阴影模块
 *
 * 职责：
 * 1. 根据光源方向 + 主相机视锥，计算光源相机参数（正交投影）
 * 2. 生成阴影深度纹理（所有几何体在光源相机下绘制）
 * 3. 为延迟光照阶段提供 ShadowMap 资源与光源 VP/参数
 */
class RENDERSYSTEM_API ShadowMapModule
{
public:
    ShadowMapModule(RenderDevice* device);
    ~ShadowMapModule();

    /**
     * @brief 渲染 ShadowMap（FrameGraph 接口）
     * @param frameGraph 帧图
     * @param commandBuffer 命令缓冲
     * @param params 渲染参数
     * @return 阴影输出（ShadowMap 资源 + 光源 VP/参数）
     */
    ShadowMapModuleOutput Render(
        const std::string& passName,
        FrameGraph& frameGraph,
        CommandBufferPtr commandBuffer,
        const ShadowMapRenderParams& params);

private:
    RenderDevice* mDevice;

    // 内部深度渲染器（复用其深度 PSO 与绘制逻辑，仅用于阴影深度）
    std::unique_ptr<DepthRenderer> mDepthRenderer;

    // 光源相机 UBO（cbPerCamera 结构，供阴影深度绘制绑定）
    UniformBufferPtr mShadowCameraUBO = nullptr;
};

typedef std::shared_ptr<ShadowMapModule> ShadowMapModulePtr;
typedef std::unique_ptr<ShadowMapModule> ShadowMapModuleUniPtr;

NS_RENDERSYSTEM_END

#endif // GNXENGINE_SHADOW_MAP_MODULE_H
