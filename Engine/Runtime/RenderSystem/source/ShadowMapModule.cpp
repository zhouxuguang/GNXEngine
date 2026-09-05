//
//  ShadowMapModule.cpp
//  GNXEngine
//
//  阴影模块实现 - 生成主方向光的 ShadowMap 深度纹理
//

#include "ShadowMapModule.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/FrameGraphTexture.h"
#include "Camera.h"
#include <tracy/Tracy.hpp>
#include <algorithm>
#include <cmath>

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

ShadowMapModule::ShadowMapModule(RenderDevice* device)
    : mDevice(device)
{
    // 复用 DepthRenderer：其 PSO 由 DepthGenerate 系列 shader 构建，
    // 只要传入"光源相机"的 cbPerCamera，就会把所有几何体在光源视角下绘制为深度图
    mDepthRenderer = std::make_unique<DepthRenderer>(device);
    if (mDepthRenderer)
    {
        DepthRenderConfig config;
        config.width = 2048;
        config.height = 2048;
        config.depthFormat = kTexFormatDepth32Float;
        mDepthRenderer->Initialize(config);
    }

    // 光源相机 UBO（cbPerCamera 结构）
    if (mDevice)
    {
        mShadowCameraUBO = mDevice->CreateUniformBufferWithSize(sizeof(cbPerCamera));
    }
}

ShadowMapModule::~ShadowMapModule()
{
}

ShadowMapModuleOutput ShadowMapModule::Render(
    const std::string& passName,
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    const ShadowMapRenderParams& params)
{
    ShadowMapModuleOutput output;
    output.valid = false;

    if (!mDevice || !mDepthRenderer || !mShadowCameraUBO)
    {
        return output;
    }

    // 没有投射体或没有方向光时，不生成阴影
    if (params.staticMeshes.empty() && params.skinnedMeshes.empty())
    {
        return output;
    }
    if (!params.light)
    {
        return output;
    }

    // ========== 1. 根据光源方向 + 主相机，构造光源相机参数 ==========
    // 方向光：光来自无穷远，用正交投影
    // 语义：getDirection() 返回"表面指向光源"的方向（与 DeferredLighting.shader
    // 中 _WorldSpaceLightPos.xyz 被当作 L = 表面→光源 的用法一致，
    // 也与 StandardBRDF.hlsl 中 lightDirection 的注释 "Vector from surface point to light" 一致）。
    // 因此真正放置光源的位置在 sceneCenter + dir * distance（沿 direction 指向光源的一侧），
    // 光从该处沿 -dir 方向照射场景。
    Vector3f lightDir = params.light->getDirection();
    lightDir.Normalize();
    if (lightDir.LengthSq() < 1e-6f)
    {
        return output;
    }

    // 阴影关注中心：优先使用主相机的注视目标（场景中心附近）
    Vector3f sceneCenter(0.0f, 0.0f, 0.0f);
    if (params.camera)
    {
        sceneCenter = params.camera->GetTarget();
    }

    // 覆盖半径：保证头盔/地面等 demo 内容在阴影范围内（可配置扩展为场景包围盒）
    const float kCoverRadius = 6.0f;
    const float shadowDistance = kCoverRadius * 2.0f; // 光源到中心的距离

    // 光源位置 = 中心 + dir * distance（光源在 direction 指向的一侧，
    // 即真实光源所在位置；lightDir 指向光源，故从中心沿 lightDir 走即到达光源）
    Vector3f lightEye = sceneCenter + lightDir * shadowDistance;

    // 上方向（避免与光方向平行）
    Vector3f up(0.0f, 1.0f, 0.0f);
    if (fabsf(up.DotProduct(lightDir)) > 0.99f)
    {
        up = Vector3f(0.0f, 0.0f, 1.0f);
    }

    // 光源视图矩阵：从光源看向场景中心
    Matrix4x4f lightView = Matrix4x4f::CreateLookAt(lightEye, sceneCenter, up);

    // 正交投影（Reverse-Z：近处 NDC z 大(1)、远处小(0)，与 DepthRenderer 的 Greater 深度测试一致）
    // 手动构造 Reverse-Z 正交矩阵：
    //   row0 = (2/(r-l), 0, 0, -(r+l)/(r-l))
    //   row1 = (0, 2/(t-b), 0, -(t+b)/(t-b))
    //   row2 = (0, 0, 1/(far-near), far/(far-near))   => z_view=-near -> 1, z_view=-far -> 0
    //   row3 = (0, 0, 0, 1)
    const float left   = -kCoverRadius;
    const float right  =  kCoverRadius;
    const float bottom = -kCoverRadius;
    const float top    =  kCoverRadius;
    const float zNear  = shadowDistance - kCoverRadius;   // 覆盖中心近端
    const float zFar   = shadowDistance + kCoverRadius;   // 覆盖中心远端
    const float invR   = 1.0f / (right - left);
    const float invT   = 1.0f / (top - bottom);
    const float invFN  = 1.0f / (zFar - zNear);

    Matrix4x4f lightProj;
    float* p = &lightProj[0][0];
    memset(p, 0, sizeof(float) * 16);
    // 布局遵循引擎 CreateReverseZPerspective/CreateOrthographic 的矩阵元素约定：
    //   p[10] = row2col2（z → clip.z 系数），p[11] = row2col3（w → clip.z 平移）
    //   p[14] = row3col2（z → clip.w，正交投影为 0），p[15] = row3col3（clip.w = w）
    // Reverse-Z：近平面(d=near, view.z=-near) clip.z=1，远平面 clip.z=0
    p[0]  = 2.0f * invR;              // x 系数
    p[3]  = -(right + left) * invR;   // x 平移
    p[5]  = 2.0f * invT;              // y 系数
    p[7]  = -(top + bottom) * invT;   // y 平移
    p[10] = invFN;                    // z 系数 = 1/(far-near)
    p[11] = zFar * invFN;             // w 平移 = far/(far-near)  (Reverse-Z)
    p[15] = 1.0f;                     // clip.w = w（正交）

    // 填充 cbPerCamera（供 DepthGenerate shader 使用）
    cbPerCamera shadowCam;
    memset(&shadowCam, 0, sizeof(shadowCam));
    shadowCam.MATRIX_P  = lightProj;
    shadowCam.MATRIX_INV_P = lightProj.Inverse();
    shadowCam.MATRIX_V  = lightView;
    shadowCam.MATRIX_INV_V = lightView.Inverse();
    shadowCam.MATRIX_VP = lightProj * lightView;
    shadowCam.MATRIX_INV_VP = shadowCam.MATRIX_VP.Inverse();
    shadowCam.WorldSpaceCameraPos = mathutil::make_simd_float3(lightEye);
    shadowCam.ProjectionParams = mathutil::make_simd_float4(1.0f, zNear, zFar, 1.0f / zFar);
    shadowCam.ScreenParams = mathutil::make_simd_float4(
        (float)params.width, (float)params.height,
        1.0f + 1.0f / (float)params.width, 1.0f + 1.0f / (float)params.height);

    mShadowCameraUBO->SetData(&shadowCam, 0, sizeof(shadowCam));

    // 保存输出：光源视图/投影矩阵 + 尺寸
    output.lightView = lightView;
    output.lightProj = lightProj;
    output.shadowMapSize = mathutil::make_simd_float4(
        (float)params.width, (float)params.height,
        1.0f / (float)params.width, 1.0f / (float)params.height);
    output.depthBias = 0.0005f;
    output.normalBias = 0.02f;
    output.lightSize = 0.02f;   // PCSS 光源大小（软阴影程度）
    output.filterRadius = 3.0f; // PCSS 搜索半径基数（纹素）
    output.valid = true;

    // ========== 2. 生成阴影深度纹理 ==========
    // 所有几何体在光源相机参数下绘制到 ShadowMap
    DepthRenderParams depthParams;
    depthParams.width = params.width;
    depthParams.height = params.height;
    depthParams.meshes.staticMeshes = params.staticMeshes;
    depthParams.meshes.skinnedMeshes = params.skinnedMeshes;
    depthParams.uniforms.cameraUBO = mShadowCameraUBO;
    depthParams.uniforms.skinnedMatrixUBO = params.skinnedMatrixUBO;

    output.shadowMap = mDepthRenderer->Render(
        passName, frameGraph, commandBuffer, depthParams, "ShadowMapTexture");

    return output;
}

NS_RENDERSYSTEM_END
