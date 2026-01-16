# DepthRenderer 深度渲染工具类

## 概述

`DepthRenderer` 是 GNXEngine 的深度渲染工具类，专门负责生成各种深度图像：

1. **ShadowMap（阴影贴图）**：用于实现实时阴影
2. **Depth Pre-Pass（深度预通过）**：用于提前剔除不可见像素，提升渲染性能

## 功能特性

### 支持的光源类型

- **DirectionalLight（平行光）**：使用级联阴影贴图（CSM，Cascaded Shadow Maps）
- **SpotLight（聚光灯）**：使用单个透视投影的ShadowMap
- **PointLight（点光源）**：使用立方体ShadowMap（6个方向）

### 核心功能

- ✅ 自动计算级联阴影分割（支持对数+均匀混合）
- ✅ 自动拟合光源投影矩阵到场景包围盒
- ✅ 立方体阴影贴图支持
- ✅ 可配置的ShadowMap分辨率和质量
- ✅ 深度预通过支持
- ✅ 线程安全设计

## 快速开始

### 1. 初始化DepthRenderer

```cpp
#include "DepthRenderer.h"

using namespace RenderSystem;

// 创建深度渲染器
DepthRenderer depthRenderer(renderDevice);

// 配置参数
DepthRenderConfig config;
config.width = 2048;              // ShadowMap分辨率
config.height = 2048;
config.cascadeCount = 4;          // 级联数量
config.nearPlane = 0.1f;         // 近裁剪面
config.farPlane = 100.0f;        // 远裁剪面
config.shadowBias = 0.005f;      // 阴影偏移
config.depthFormat = TextureFormat::Depth32Float;
config.cascadeSplitLambda = 0.5f; // 对数+均匀混合系数

// 初始化
if (!depthRenderer.Initialize(config)) {
    LOG_ERROR("Failed to initialize DepthRenderer");
}
```

### 2. 渲染DirectionalLight的阴影

```cpp
// 渲染阴影贴图
depthRenderer.RenderShadowMap(directionalLight, camera, scene, commandBuffer);

// 获取阴影贴图
RCTexturePtr shadowMap = depthRenderer.GetShadowMap(directionalLight);

// 获取级联的视图投影矩阵（传递给着色器）
for (uint32_t i = 0; i < 4; i++) {
    Matrix4x4f viewMatrix = depthRenderer.GetLightViewMatrix(directionalLight, i);
    Matrix4x4f projMatrix = depthRenderer.GetLightProjectionMatrix(directionalLight, i);
    Matrix4x4f vpMatrix = projMatrix * viewMatrix;
    
    float splitDist = depthRenderer.GetCascadeSplitDistance(i);
    
    // 设置到Uniform Buffer
    uniformBuffer->SetLightVPMatrix(i, vpMatrix);
    uniformBuffer->SetCascadeSplitDistance(i, splitDist);
}
```

### 3. 渲染SpotLight的阴影

```cpp
// 渲染阴影贴图
depthRenderer.RenderShadowMap(spotLight, camera, scene, commandBuffer);

// 获取阴影贴图和VP矩阵
RCTexturePtr shadowMap = depthRenderer.GetShadowMap(spotLight);
Matrix4x4f vpMatrix = depthRenderer.GetLightProjectionMatrix(spotLight, 0) * 
                       depthRenderer.GetLightViewMatrix(spotLight, 0);
```

### 4. 渲染PointLight的阴影

```cpp
// 渲染立方体阴影贴图
depthRenderer.RenderShadowMap(pointLight, nullptr, scene, commandBuffer);

// 获取阴影贴图
RCTexturePtr cubeShadowMap = depthRenderer.GetShadowMap(pointLight);

// PointLight需要在着色器中使用立方体采样
shader->SetCubeShadowTexture(cubeShadowMap);
shader->SetLightPosition(pointLight->getPosition());
```

### 5. 深度预通过

```cpp
// 配置参数（通常与屏幕分辨率相同）
DepthRenderConfig depthPrePassConfig;
depthPrePassConfig.width = 1920;   // 屏幕宽度
depthPrePassConfig.height = 1080;  // 屏幕高度
depthPrePassConfig.nearPlane = camera->GetNearZ();
depthPrePassConfig.farPlane = camera->GetFarZ();
depthPrePassConfig.depthFormat = TextureFormat::Depth24Stencil8;

DepthRenderer depthPrePassRenderer(renderDevice);
depthPrePassRenderer.Initialize(depthPrePassConfig);

// 渲染深度预通过
depthPrePassRenderer.RenderDepthPrePass(camera, scene, commandBuffer);

// 获取深度纹理
RCTexturePtr depthTexture = depthPrePassRenderer.GetDepthTexture();

// 后续渲染可以重用深度值
depthStencilState.depthCompareFunction = CompareFunctionLessEqual;
depthStencilState.depthWriteEnabled = false;
```

## 配置参数详解

### DepthRenderConfig

```cpp
struct DepthRenderConfig {
    uint32_t width = 1024;              // 深度纹理宽度
    uint32_t height = 1024;             // 深度纹理高度
    uint32_t cascadeCount = 4;          // 级联阴影贴图数量
    float nearPlane = 0.1f;             // 近裁剪面距离
    float farPlane = 100.0f;            // 远裁剪面距离
    float shadowBias = 0.005f;          // 阴影偏移
    TextureFormat depthFormat = TextureFormat::Depth32Float;  // 深度格式
    float cascadeSplitLambda = 0.5f;     // 级联分割系数
};
```

#### 参数说明

- **width/height**：ShadowMap分辨率，越高阴影质量越好，但性能开销越大
  - DirectionalLight：2048x2048（高质量）
  - SpotLight：1024x1024
  - PointLight：512x512（每个面）

- **cascadeCount**：级联阴影贴图数量
  - 推荐值：3-4
  - 数量越多，近处阴影质量越好，但开销越大

- **cascadeSplitLambda**：级联分割系数
  - 0.0：完全对数分割（适合近处质量要求高）
  - 0.5：对数+均匀混合（推荐）
  - 1.0：完全均匀分割（适合远处质量要求高）

- **shadowBias**：阴影偏移，防止阴影痤疮（shadow acne）
  - 值太小：出现痤疮
  - 值太大：出现Peter Panning（物体脱离阴影）

- **depthFormat**：深度格式
  - `Depth32Float`：32位浮点，精度最高，用于高质量阴影
  - `Depth24Stencil8`：24位深度+8位模板，用于Depth Pre-Pass
  - `Depth16`：16位，节省带宽，质量较低

## 完整渲染流程示例

### 阴影渲染流程

```cpp
// 阶段1：渲染阴影贴图
DepthRenderer depthRenderer(renderDevice);
DepthRenderConfig config;
config.width = 2048;
config.height = 2048;
config.cascadeCount = 4;
config.shadowBias = 0.005f;

depthRenderer.Initialize(config);

// 渲染每个光源的阴影
for (auto light : scene->GetLights()) {
    depthRenderer.RenderShadowMap(light, camera, scene, commandBuffer);
}

// 阶段2：深度预通过
depthRenderer.RenderDepthPrePass(camera, scene, commandBuffer);

// 阶段3：使用阴影渲染场景
auto renderEncoder = commandBuffer->CreateRenderEncoder(mainRenderPass);

// 设置管线和资源
renderEncoder->SetGraphicsPipeline(shadowPipeline);
renderEncoder->SetFragmentTexture("shadowMap", shadowMap, sampler);

// 渲染场景
scene->Render(renderEncoder);

renderEncoder->EndEncoding();
```

### PCF软阴影实现

在着色器中实现PCF（Percentage Closer Filtering）以获得更柔和的阴影边缘：

```glsl
// 顶点着色器
vec4 FragPosLightSpace[4];  // 4个级联

// 片段着色器
float CalculateShadowPCF(int cascadeIndex, vec3 fragPos) {
    vec4 fragPosLightSpace = LightVPs[cascadeIndex] * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMaps[cascadeIndex], 0);
    
    // 3x3 PCF
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float pcfDepth = texture(shadowMaps[cascadeIndex], 
                                    projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z > pcfDepth) ? 1.0 : 0.0;
        }
    }
    
    return shadow / 9.0;
}
```

## 性能优化建议

### 1. 动态调整阴影质量

```cpp
float cameraDistance = (camera->GetPosition() - target).Length();

if (cameraDistance < 50.0f) {
    config.width = 4096;
    config.height = 4096;
    config.cascadeCount = 4;
} else if (cameraDistance < 100.0f) {
    config.width = 2048;
    config.height = 2048;
    config.cascadeCount = 3;
} else {
    config.width = 1024;
    config.height = 1024;
    config.cascadeCount = 2;
}
```

### 2. 阴影贴图缓存

```cpp
// 只在光源移动时重新渲染阴影
if (light->HasMoved()) {
    depthRenderer.RenderShadowMap(light, camera, scene, commandBuffer);
    light->MarkAsStatic();
}
```

### 3. 深度预通过优化

```cpp
// Depth Pre-Pass只需要深度，禁用颜色写入
ColorAttachmentDescriptor colorDesc = {};
colorDesc.pixelFormat = TextureFormat::Invalid;  // 无颜色附件

DepthStencilDescriptor depthDesc = {};
depthDesc.depthCompareFunction = CompareFunctionLess;
depthDesc.depthWriteEnabled = true;

RenderPassDescriptor desc = {};
desc.depthAttachment = depthDesc;
```

## 注意事项

1. **线程安全**：`DepthRenderer` 不是线程安全的，确保在单个线程中使用
2. **资源管理**：`DepthRenderer` 使用智能指针管理资源，析构时自动释放
3. **Camera参数**：DirectionalLight需要Camera参数计算级联，其他光源可为nullptr
4. **内存占用**：高分辨率ShadowMap占用大量显存，注意控制
5. **着色器集成**：需要相应的着色器支持阴影采样和深度预通过

## 更多示例

详细的使用示例请参考：
- `DepthRendererExample.h`：包含7个完整的使用示例
  - DirectionalLight阴影渲染
  - SpotLight阴影渲染
  - PointLight阴影渲染
  - Depth Pre-Pass
  - 完整阴影渲染流程
  - 动态阴影质量调整
  - PCF软阴影

## API参考

### 主要方法

| 方法 | 说明 |
|------|------|
| `Initialize(config)` | 初始化深度渲染器 |
| `RenderShadowMap(light, camera, scene, cmdBuffer)` | 渲染阴影贴图 |
| `RenderDepthPrePass(camera, scene, cmdBuffer)` | 渲染深度预通过 |
| `GetShadowMap(light)` | 获取阴影贴图 |
| `GetDepthTexture()` | 获取深度纹理 |
| `GetLightViewMatrix(light, cascadeIndex)` | 获取光源视图矩阵 |
| `GetLightProjectionMatrix(light, cascadeIndex)` | 获取光源投影矩阵 |
| `GetCascadeSplitDistance(cascadeIndex)` | 获取级联分割距离 |
| `UpdateConfig(config)` | 更新配置参数 |
| `Shutdown()` | 关闭并释放资源 |

## 故障排除

### 问题：阴影有痤疮（Shadow Acne）

**解决方案**：增加 `shadowBias` 值
```cpp
config.shadowBias = 0.01f;  // 从0.005f增加到0.01f
```

### 问题：阴影偏移（Peter Panning）

**解决方案**：减小 `shadowBias` 值
```cpp
config.shadowBias = 0.001f;  // 从0.005f减小到0.001f
```

### 问题：远处阴影质量差

**解决方案**：调整级联分割系数
```cpp
config.cascadeSplitLambda = 0.7f;  // 增加均匀分割比例
```

### 问题：近处阴影质量差

**解决方案**：增加级联数量
```cpp
config.cascadeCount = 5;  // 从4增加到5
```

## 版本历史

- **v1.0** (2025/1/11)
  - 初始版本
  - 支持DirectionalLight/SpotLight/PointLight阴影
  - 支持级联阴影贴图
  - 支持深度预通过

## 许可证

本代码是 GNXEngine 的一部分，遵循项目的许可证。
