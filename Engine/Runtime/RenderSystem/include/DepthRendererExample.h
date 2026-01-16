//
//  DepthRendererExample.h
//  GNXEngine
//
//  Created by AI Assistant on 2025/1/11.
//  DepthRenderer使用示例
//

#ifndef GNX_ENGINE_DEPTH_RENDERER_EXAMPLE_INCLUDE_H
#define GNX_ENGINE_DEPTH_RENDERER_EXAMPLE_INCLUDE_H

#include "DepthRenderer.h"
#include "Camera.h"
#include "Light.h"
#include "Runtime/RenderCore/include/CommandBuffer.h"

NS_RENDERSYSTEM_BEGIN

/**
 * @brief DepthRenderer使用示例
 * 
 * 展示如何使用DepthRenderer类生成：
 * 1. ShadowMap（阴影贴图）
 * 2. Depth Pre-Pass（深度预通过）
 */
class DepthRendererExample {
public:
    /**
     * @brief 示例1：渲染DirectionalLight的级联阴影贴图
     */
    static void RenderDirectionalLightShadowExample(RenderDevice* device, 
                                                    CommandBuffer* cmdBuffer,
                                                    DirectionLight* dirLight,
                                                    Scene* scene,
                                                    Camera* camera)
    {
        // 1. 创建深度渲染器
        DepthRenderer depthRenderer(device);
        
        // 2. 配置参数
        DepthRenderConfig config;
        config.width = 2048;              // ShadowMap分辨率
        config.height = 2048;
        config.cascadeCount = 4;          // 4个级联
        config.nearPlane = camera->GetNearZ();
        config.farPlane = camera->GetFarZ();
        config.shadowBias = 0.005f;
        config.depthFormat = TextureFormat::Depth32Float;
        config.cascadeSplitLambda = 0.5f; // 对数+均匀混合
        
        // 3. 初始化
        if (!depthRenderer.Initialize(config)) {
            // 处理初始化失败
            return;
        }
        
        // 4. 渲染阴影贴图
        depthRenderer.RenderShadowMap(dirLight, scene, cmdBuffer);
        
        // 5. 获取阴影贴图
        RCTexturePtr shadowMap = depthRenderer.GetShadowMap(dirLight);
        
        // 6. 获取级联的视图投影矩阵（传递给着色器）
        for (uint32_t i = 0; i < 4; i++) {
            Matrix4x4f viewMatrix = depthRenderer.GetLightViewMatrix(dirLight, i);
            Matrix4x4f projMatrix = depthRenderer.GetLightProjectionMatrix(dirLight, i);
            Matrix4x4f vpMatrix = projMatrix * viewMatrix;
            
            float splitDist = depthRenderer.GetCascadeSplitDistance(i);
            
            // 设置到Uniform Buffer传递给着色器
            // uniformBuffer->SetLightVPMatrix(i, vpMatrix);
            // uniformBuffer->SetCascadeSplitDistance(i, splitDist);
        }
        
        // 7. 使用阴影贴图渲染场景
        // 着色器中会采样阴影贴图并比较深度
        // shader->SetShadowTexture(shadowMap);
        // scene->RenderWithShadows(cmdBuffer);
    }
    
    /**
     * @brief 示例2：渲染SpotLight的阴影贴图
     */
    static void RenderSpotLightShadowExample(RenderDevice* device,
                                            CommandBuffer* cmdBuffer,
                                            SpotLight* spotLight,
                                            Scene* scene)
    {
        // 创建深度渲染器
        DepthRenderer depthRenderer(device);
        
        // 配置参数
        DepthRenderConfig config;
        config.width = 1024;
        config.height = 1024;
        config.nearPlane = 0.1f;
        config.farPlane = 100.0f;
        config.shadowBias = 0.005f;
        
        depthRenderer.Initialize(config);
        
        // 渲染阴影贴图
        depthRenderer.RenderShadowMap(spotLight, scene, cmdBuffer);
        
        // 获取阴影贴图和VP矩阵
        RCTexturePtr shadowMap = depthRenderer.GetShadowMap(spotLight);
        Matrix4x4f viewMatrix = depthRenderer.GetLightViewMatrix(spotLight);
        Matrix4x4f projMatrix = depthRenderer.GetLightProjectionMatrix(spotLight);
        Matrix4x4f vpMatrix = projMatrix * viewMatrix;
        
        // 使用阴影贴图
        // shader->SetShadowTexture(shadowMap);
        // shader->SetLightVPMatrix(vpMatrix);
    }
    
    /**
     * @brief 示例3：渲染PointLight的立方体阴影贴图
     */
    static void RenderPointLightShadowExample(RenderDevice* device,
                                             CommandBuffer* cmdBuffer,
                                             PointLight* pointLight,
                                             Scene* scene)
    {
        // 创建深度渲染器
        DepthRenderer depthRenderer(device);
        
        // 配置参数
        DepthRenderConfig config;
        config.width = 512;   // 立方体贴图每个面较小
        config.height = 512;
        config.nearPlane = 0.1f;
        config.farPlane = 50.0f;
        config.shadowBias = 0.005f;
        
        depthRenderer.Initialize(config);
        
        // 渲染阴影贴图（6个面）
        depthRenderer.RenderShadowMap(pointLight, scene, cmdBuffer);
        
        // 获取阴影贴图
        RCTexturePtr shadowMap = depthRenderer.GetShadowMap(pointLight);
        
        // 使用立方体阴影贴图
        // shader->SetCubeShadowTexture(shadowMap);
        // shader->SetLightPosition(pointLight->getPosition());
    }
    
    /**
     * @brief 示例4：深度预通过（Depth Pre-Pass）
     * 
     * Depth Pre-Pass可以在正式渲染之前提前深度测试，
     * 避免渲染不可见的像素，提升性能。
     */
    static void RenderDepthPrePassExample(RenderDevice* device,
                                          CommandBuffer* cmdBuffer,
                                          Camera* camera,
                                          Scene* scene)
    {
        // 创建深度渲染器
        DepthRenderer depthRenderer(device);
        
        // 配置参数（通常与屏幕分辨率相同）
        DepthRenderConfig config;
        config.width = 1920;   // 屏幕宽度
        config.height = 1080;  // 屏幕高度
        config.nearPlane = camera->GetNearZ();
        config.farPlane = camera->GetFarZ();
        config.depthFormat = TextureFormat::Depth24Stencil8;
        
        depthRenderer.Initialize(config);
        
        // 渲染深度预通过
        depthRenderer.RenderDepthPrePass(camera, scene, cmdBuffer);
        
        // 获取深度纹理
        RCTexturePtr depthTexture = depthRenderer.GetDepthTexture();
        
        // 使用深度纹理
        // 可以用于：
        // 1. SSAO（屏幕空间环境光遮蔽）
        // 2. Depth of Field（景深效果）
        // 3. Screen Space Reflections（屏幕空间反射）
        // shader->SetDepthTexture(depthTexture);
        
        // 后续的正式渲染可以使用深度预通过的结果
        // 设置深度测试模式为LessEqual（重用深度值）
        // depthStencilState.depthCompareFunction = CompareFunctionLessEqual;
        // depthStencilState.depthWriteEnabled = false;
    }
    
    /**
     * @brief 示例5：完整的阴影渲染流程
     */
    static void CompleteShadowRenderingExample(RenderDevice* device,
                                               CommandBuffer* cmdBuffer,
                                               DirectionLight* dirLight,
                                               Scene* scene,
                                               Camera* camera)
    {
        // === 阶段1：渲染阴影贴图 ===
        DepthRenderer depthRenderer(device);
        
        DepthRenderConfig config;
        config.width = 2048;
        config.height = 2048;
        config.cascadeCount = 4;
        config.nearPlane = 0.1f;
        config.farPlane = 100.0f;
        config.shadowBias = 0.005f;
        
        depthRenderer.Initialize(config);
        depthRenderer.RenderShadowMap(dirLight, scene, cmdBuffer);
        
        // === 阶段2：深度预通过 ===
        depthRenderer.RenderDepthPrePass(camera, scene, cmdBuffer);
        
        // === 阶段3：渲染场景（使用阴影） ===
        // 创建主渲染通道
        RenderPassDescriptor mainPassDesc = {};
        // ... 配置颜色和深度附件
        
        auto renderEncoder = cmdBuffer->CreateRenderEncoder(mainPassDesc);
        
        // 设置管线和资源
        // renderEncoder->SetGraphicsPipeline(shadowPipeline);
        // renderEncoder->SetFragmentTexture("shadowMap", depthRenderer.GetShadowMap(dirLight), sampler);
        // renderEncoder->SetFragmentTexture("depthTexture", depthRenderer.GetDepthTexture(), sampler);
        
        // 设置光照和阴影参数
        for (uint32_t i = 0; i < 4; i++) {
            Matrix4x4f lightVP = depthRenderer.GetLightProjectionMatrix(dirLight, i) * 
                                 depthRenderer.GetLightViewMatrix(dirLight, i);
            // uniformBuffer->SetCascadeLightVP(i, lightVP);
        }
        
        // 渲染场景
        // scene->Render(renderEncoder, camera->GetViewMatrix(), camera->GetProjectionMatrix());
        
        renderEncoder->EndEncoding();
    }
    
    /**
     * @brief 示例6：动态调整阴影质量
     */
    static void AdaptiveShadowQualityExample(RenderDevice* device,
                                            CommandBuffer* cmdBuffer,
                                            DirectionLight* dirLight,
                                            Scene* scene,
                                            Camera* camera)
    {
        DepthRenderer depthRenderer(device);
        DepthRenderConfig config;
        
        // 根据距离动态调整阴影质量
        float cameraDist = (camera->GetPosition() - Vector3f::ZERO).Length();
        
        if (cameraDist < 50.0f) {
            // 高质量：高分辨率，多级联
            config.width = 4096;
            config.height = 4096;
            config.cascadeCount = 4;
        } else if (cameraDist < 100.0f) {
            // 中等质量
            config.width = 2048;
            config.height = 2048;
            config.cascadeCount = 3;
        } else {
            // 低质量
            config.width = 1024;
            config.height = 1024;
            config.cascadeCount = 2;
        }
        
        config.nearPlane = camera->GetNearZ();
        config.farPlane = camera->GetFarZ();
        config.shadowBias = 0.005f;
        
        depthRenderer.Initialize(config);
        depthRenderer.RenderShadowMap(dirLight, scene, cmdBuffer);
    }
    
    /**
     * @brief 示例7：使用ShadowMap实现PCF软阴影
     * 
     * PCF (Percentage Closer Filtering) 通过采样多个点来计算阴影，
     * 产生更柔和的边缘。
     */
    static void PCFSoftShadowExample(RenderDevice* device,
                                      CommandBuffer* cmdBuffer,
                                      DirectionLight* dirLight,
                                      Scene* scene,
                                      Camera* camera)
    {
        // 渲染阴影贴图
        DepthRenderer depthRenderer(device);
        
        DepthRenderConfig config;
        config.width = 2048;
        config.height = 2048;
        config.cascadeCount = 4;
        
        depthRenderer.Initialize(config);
        depthRenderer.RenderShadowMap(dirLight, scene, cmdBuffer);
        
        // PCF在着色器中实现
        // 着色器伪代码：
        /*
        float CalculateShadowPCF(vec3 fragPos, vec3 lightDir, sampler2D shadowMap, mat4 lightVP) {
            vec4 fragPosLightSpace = lightVP * vec4(fragPos, 1.0);
            vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
            projCoords = projCoords * 0.5 + 0.5;
            
            float shadow = 0.0;
            float texelSize = 1.0 / textureSize(shadowMap, 0).x;
            
            // 3x3 PCF
            for (int x = -1; x <= 1; x++) {
                for (int y = -1; y <= 1; y++) {
                    float pcfDepth = texture(shadowMap, projCoords.xy + 
                                             vec2(x, y) * texelSize).r;
                    shadow += projCoords.z > pcfDepth ? 1.0 : 0.0;
                }
            }
            
            return shadow / 9.0;  // 平均值
        }
        */
    }
};

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_DEPTH_RENDERER_EXAMPLE_INCLUDE_H */
