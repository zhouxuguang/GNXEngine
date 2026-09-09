# GNXEngine 跨平台 Shader 打包系统分析与设计

## 结论

当前 Shader 系统的源码位置基本合理，但编译产物位置、平台隔离、工程级 Shader、依赖追踪和加载命名空间仍需重新设计。

现有链路是：

```text
Engine/Shader/built-in/*.shader
        ↓ shader_compile
data_asset/Shader/{name}.{format}.gnxasset
        ↓
LoadShaderAsset("{name}")
        ↓
根据 RenderDevice 拼接格式后缀并加载
```

例如：

```text
Engine/Shader/built-in/PBR.shader
    ↓
data_asset/Shader/PBR.spirv.gnxasset
data_asset/Shader/PBR.msl_ios.gnxasset
data_asset/Shader/PBR.msl_macos.gnxasset
```

运行时根据渲染后端选择格式：

- Vulkan → SPIR-V
- iOS Metal → MSL iOS
- macOS Metal → MSL macOS

相关实现：

- `Engine/Runtime/RenderSystem/source/ShaderAssetLoader.cpp:24`
- `Engine/Runtime/RenderSystem/source/ShaderAssetLoader.cpp:111`

这个基础思路可用，但还不是完整的跨平台 Shader Cook 系统。

## 当前放置位置是否合理

### 源码位置合理

`Engine/Shader/built-in/` 作为引擎内置 Shader 源码目录是合理的，适合放置：

- BasePass
- GBuffer
- DeferredLighting
- SSAO
- Skybox
- Terrain
- Nanite
- Virtual Texture
- Atmosphere
- 引擎公共 HLSL include

这些 Shader 属于引擎实现，而非具体游戏工程，不应放入工程的 `Assets`。

现有子目录结构也基本合理：

```text
Engine/Shader/built-in/
├─ Atmosphere/
├─ MeshShader/
├─ Nanite/
├─ vt/
├─ GNXEngineCommon.hlsl
├─ GNXEngineVariables.hlsl
└─ PBR.shader
```

以后可以规范成：

```text
Engine/Shader/
├─ built-in/
└─ include/
```

### 编译产物位置不合理

当前全部输出到：

```text
data_asset/Shader/
```

批量脚本中使用的是固定路径：

- `tools/compile_shaders.ps1`
- `tools/compile_shaders.sh`

主要问题包括：

- 所有平台格式混在同一目录。
- 引擎 Shader 和未来工程 Shader 会混在一起。
- `data_asset` 同时承担缓存、暂存和发布资源三种职责。
- 旧编译产物可能残留并进入发布包。
- 无法确定某次构建真正需要哪些 Shader。
- Android/iOS 会把整个公共目录复制进包。
- 无法可靠做增量 Cook、依赖检查和包体统计。

目前仓库中实际只有约 40 个 SPIR-V 产物，没有 MSL/DXIL。因此，当前源码布局支持多格式，但现有资产目录没有准备好完整的跨平台发布产物。

## 当前实现存在的问题

### 1. Shader 缓存按文件名索引

`AssetManager::LoadShader()` 从完整路径中只提取文件名作为缓存 key：

- `Engine/Runtime/AssetManager/source/AssetManager.cpp:472`

因此下面两个 Shader 可能发生冲突：

```text
Engine/PBR.spirv.gnxasset
Project/PBR.spirv.gnxasset
```

或者：

```text
Project/Characters/Skin.spirv.gnxasset
Project/Environment/Skin.spirv.gnxasset
```

虽然格式后缀避免了 SPIR-V 与 MSL 互相串缓存，但目录命名空间仍然丢失。

缓存 key 至少应为规范化逻辑路径，最终应当是：

```text
Shader GUID + VariantKey + PlatformProfile
```

### 2. 工程级 Shader 没有独立入口

当前调用：

```cpp
LoadShaderAsset("Nanite/ClusterCull");
```

默认只认为它是引擎内置 Shader，并固定从 `Shader/` 下寻找。

桌面端预编译产物缺失时，还会访问：

```text
Engine/Shader/built-in/{shaderName}.shader
```

执行运行时编译。

因此，当前接口实际上等价于：

```cpp
LoadBuiltinShaderAsset(name);
```

它不能清晰表达：

- 引擎 Shader
- 工程 Shader
- 插件 Shader

### 3. include 系统不适合工程扩展

现有 Shader 大量使用相对 include：

```hlsl
#include "GNXEngineCommon.hlsl"
#include "../GNXEngineVariables.hlsl"
```

DXC 使用默认 include handler，目前没有显式传入工程和引擎 include root：

- `Engine/Runtime/ShaderCompiler/source/DXCompilerUtil.cpp:183`

如果工程 Shader 需要引用引擎公共定义，就可能写出依赖目录层级的相对路径。这类路径不可移植，也无法建立稳定的依赖关系。

### 4. 缺少 Shader Variant 维度

目前 Cook key 主要围绕：

```text
shaderName + format
```

但实际 Shader 产物通常还受以下因素影响：

- 宏定义组合
- Render Pass
- 材质功能
- 骨骼动画
- Instancing
- Alpha Test
- Normal Map
- MSAA
- Reverse-Z
- Bindless
- Mesh Shader 开关
- 平台能力等级
- 编译器版本
- 优化等级

例如：

```text
PBR + NORMAL_MAP + ALPHA_TEST
PBR + SKINNED
PBR + INSTANCING
```

这些必须是不同的 Cook 产物。

### 5. include 变化可能无法触发增量编译

Shader 编译结果不能只根据主 `.shader` 文件计算 hash。以下任意 include 改变，都必须让依赖它的 Shader 重新 Cook：

```text
GNXEngineVariables.hlsl
GNXEngineCommon.hlsl
GBufferCommon.hlsl
StandardBRDF.hlsl
```

Cook 系统需要解析递归 include，并把所有依赖文件内容纳入 `cookHash`。

### 6. 反射数据尚未完整写入容器

`ShaderMessage.proto` 定义了：

- uniform buffers
- resources
- push constants
- vertex inputs

但 `ShaderPackageBuilder` 当前主要填充顶点输入和 push constant：

- `Engine/Runtime/AssetManager/source/ShaderPackageBuilder.cpp`

工程材质 Shader 后续需要可靠的资源反射，例如：

```text
gBaseColorTexture → set 1, binding 0
gBaseColorSampler → set 1, binding 1
MaterialParams    → set 1, binding 2
```

否则 Material 很难通过名称自动绑定不同平台下的资源。

## 推荐源码目录

建议严格区分引擎级与工程级 Shader：

```text
GNXEngine/
├─ Engine/
│  └─ Shader/
│     ├─ built-in/
│     │  ├─ Rendering/
│     │  ├─ PostProcess/
│     │  ├─ Terrain/
│     │  ├─ Nanite/
│     │  └─ VirtualTexture/
│     └─ include/
│        ├─ GNXEngineCommon.hlsl
│        ├─ GNXEngineVariables.hlsl
│        ├─ Lighting.hlsl
│        └─ StandardBRDF.hlsl
│
└─ Project/
   └─ Assets/
      └─ Shaders/
         ├─ Materials/
         │  ├─ Toon.shader
         │  └─ Water.shader
         ├─ PostProcess/
         │  └─ ColorGrading.shader
         ├─ Compute/
         │  └─ GrassCull.shader
         └─ Include/
            └─ ProjectLighting.hlsl
```

对应两类逻辑资产 ID：

```text
engine://shader/Rendering/BasePass
engine://shader/PostProcess/SSAO
project://shader/Materials/Toon
project://shader/Compute/GrassCull
```

不要让项目通过同名文件隐式覆盖引擎 Shader。需要覆盖时，应由配置明确声明：

```yaml
overrides:
  engine://shader/Rendering/BasePass:
    with: project://shader/Rendering/CustomBasePass
```

这样不会因为工程中恰好存在一个 `PBR.shader` 就无意覆盖引擎实现。

## Shader include 目录设计

编译器应支持多个明确的 include root：

```text
-I Engine/Shader/include
-I Engine/Shader/built-in
-I Project/Assets/Shaders/Include
-I Project/Assets/Shaders
```

Shader 中使用虚拟命名空间更理想：

```hlsl
#include <Engine/GNXEngineCommon.hlsl>
#include <Engine/Lighting.hlsl>
#include <Project/ProjectLighting.hlsl>
```

对应解析规则：

```text
Engine/*  → Engine/Shader/include/*
Project/* → Project/Assets/Shaders/Include/*
```

不建议允许工程 Shader 通过任意 `../` 跳出 Shader 根目录，否则会导致：

- 构建不可复现。
- 依赖扫描困难。
- 不同构建机结果不同。
- 工程 Shader 与源码目录结构强耦合。

每次编译应记录递归依赖：

```json
{
  "source": "project://shader/Materials/Toon",
  "dependencies": [
    "Engine/GNXEngineCommon.hlsl",
    "Engine/Lighting.hlsl",
    "Project/ProjectLighting.hlsl"
  ]
}
```

## 跨平台 Shader Cook 目录

与纹理一样，不能直接把 Cook 结果写入 `data_asset`。

推荐：

```text
Library/DerivedData/Shaders/
├─ Windows-Vulkan/
│  └─ {cookHash}.gnxasset
├─ Windows-DX12/
│  └─ {cookHash}.gnxasset
├─ Android-Vulkan/
│  └─ {cookHash}.gnxasset
├─ iOS-Metal/
│  └─ {cookHash}.gnxasset
└─ macOS-Metal/
   └─ {cookHash}.gnxasset
```

目标格式映射：

| Platform Profile | Shader 格式 |
|---|---|
| Windows-Vulkan | SPIR-V |
| Windows-DX12 | DXIL |
| Android-Vulkan | SPIR-V |
| iOS-Metal | MSL iOS，后续可改 Metal Library |
| macOS-Metal | MSL macOS，后续可改 Metal Library |

当前运行时 `GetShaderFormat()` 只明确处理 Metal 和 Vulkan。虽然编译工具宣称支持 DXIL，但运行时尚未接入 D3D12 格式选择。因此 DXIL 目前更接近预留能力，并非完整可发布链路。

## Shader Cook key

建议：

```text
cookHash =
    hash(
        主 Shader 内容
        + 所有递归 include 内容
        + Shader GUID
        + stage 列表
        + entry points
        + variant defines
        + TargetPlatform
        + RenderBackend
        + ShaderFormat
        + shader model
        + compiler version
        + compiler flags
        + Reverse-Z 等全局配置
        + binding layout version
    )
```

不能只包含主文件的 source hash。

## Shader Variant 设计

建议定义：

```cpp
struct ShaderVariantKey
{
    ShaderId shader;
    uint64_t keywordMask;
    RenderPassType pass;
    PlatformProfile platform;
    uint32_t pipelineLayoutVersion;
};
```

工程 Shader 的 meta 可以这样描述：

```yaml
guid: "shader-toon-guid"
type: Shader

source:
  file: Shaders/Materials/Toon.shader
  stages:
    vertex: VS
    fragment: PS

variants:
  keywords:
    - NORMAL_MAP
    - ALPHA_TEST
    - SKINNED

platformOverrides:
  Android:
    excludeKeywords:
      - TESSELLATION
      - MESH_SHADER
```

不要把所有关键字做笛卡尔积，否则变体数量会爆炸。应从以下来源收集真正使用的变体：

- Material 引用
- Render pipeline 固定需求
- Scene 依赖
- 显式 always-include 列表

例如工程中只使用：

```text
Toon
Toon + NORMAL_MAP
Toon + SKINNED
```

就只打包这三个变体。

## 平台 Staging 设计

最终平台目录只放该平台真正需要的格式：

```text
Build/Staging/Android/
├─ AssetManifest.bin
└─ Content/
   └─ Shaders/
      ├─ Engine/
      │  ├─ Rendering/BasePass.gnxasset
      │  └─ PostProcess/SSAO.gnxasset
      └─ Project/
         ├─ Materials/Toon/
         │  ├─ default.gnxasset
         │  └─ normal_map.gnxasset
         └─ Compute/GrassCull/
            └─ default.gnxasset
```

Windows-DX12 对应文件内容是 DXIL，Android 是 SPIR-V，iOS 是 MSL，但逻辑路径保持一致：

```text
Content/Shaders/Project/Materials/Toon/default.gnxasset
```

因为每个包只包含一个平台格式，所以发布 Staging 中不再需要：

```text
Toon.spirv.gnxasset
Toon.msl_ios.gnxasset
Toon.dxil.gnxasset
```

格式后缀适合开发期 DerivedData，但平台 Staging 中可以去掉，因为目标平台已经确定。

## Manifest 设计

每个平台生成独立 Shader manifest：

```json
{
  "platform": "Android-Vulkan",
  "shaders": {
    "project://shader/Materials/Toon": {
      "guid": "shader-toon-guid",
      "variants": {
        "00000000": {
          "path": "Content/Shaders/Project/Materials/Toon/default.gnxasset",
          "format": "SPIRV",
          "stages": ["Vertex", "Fragment"],
          "contentHash": "..."
        },
        "00000001": {
          "path": "Content/Shaders/Project/Materials/Toon/normal_map.gnxasset",
          "format": "SPIRV",
          "stages": ["Vertex", "Fragment"],
          "contentHash": "..."
        }
      }
    }
  }
}
```

Material 不应保存物理文件路径，而应保存：

```yaml
shader:
  guid: "shader-toon-guid"
  variantKeywords:
    - NORMAL_MAP
```

打包器通过 Material → Shader Variant 建立依赖，并把需要的 Shader 产物放入 Staging。

## 运行时加载设计

建议区分底层和上层接口：

```cpp
class ShaderLibrary
{
public:
    ShaderHandle Load(
        ShaderId shader,
        const ShaderVariantKey& variant);

    ShaderHandle LoadBuiltin(
        const std::string& logicalName);

    ShaderHandle LoadProject(
        const AssetGUID& guid,
        const ShaderVariantKey& variant);
};
```

调用示例：

```cpp
auto basePass =
    shaderLibrary.LoadBuiltin("Rendering/BasePass");

auto toon =
    shaderLibrary.LoadProject(
        material.shaderGuid,
        material.BuildVariantKey());
```

内部流程：

```text
ShaderId + VariantKey
        ↓
查询当前平台 AssetManifest
        ↓
得到唯一的 .gnxasset 相对路径
        ↓
AssetProvider 读取
        ↓
验证格式、stage、版本和 hash
        ↓
创建 VkShaderModule / MTLFunction / DXIL Shader
        ↓
按 ShaderId + VariantKey 缓存
```

运行时不应再根据文件名猜测：

```cpp
name + ".spirv.gnxasset"
name + ".msl_ios.gnxasset"
```

格式选择应发生在 Cook/Manifest 阶段。运行时只加载当前平台 manifest 指向的唯一产物。

### 开发模式 fallback

桌面编辑器可以保留运行时编译，但应明确限定：

```text
Editor/Development：
    预编译产物缺失 → 在线编译 → 写入 DerivedData

Shipping：
    预编译产物缺失 → 立即报错
```

移动端和 Shipping 构建不应尝试运行时编译。

## 引擎级与工程级 Shader 一起打包

建议构建过程为：

```text
1. 读取目标平台 Profile
2. 扫描引擎 always-include Shader
3. 从启动 Scene 开始收集工程资产依赖
4. 从 Material 收集工程 Shader 和 Variant
5. 合并显式 Shader Collection
6. Cook 所有需要的 Shader
7. 验证各 Shader 的 stage 和平台格式
8. 写入平台 Staging
9. 生成 AssetManifest
10. APK/IPA/EXE 只复制该 Staging
```

引擎级 Shader 可分为：

```text
Always Included
├─ BasePass
├─ DepthGenerate
├─ ErrorShader
└─ UI

Feature Included
├─ SSAO
├─ Terrain
├─ Atmosphere
├─ Nanite
└─ VirtualTexture
```

只有启用对应渲染功能时，才打包 Feature Shader。

工程级 Shader 来自：

- Material 引用
- Render feature 配置
- Compute task 引用
- 显式 Shader Collection

为了处理动态加载，可提供：

```yaml
shaderCollections:
  - name: MainGame
    shaders:
      - project://shader/Materials/Toon
      - project://shader/Materials/Water
```

## 推荐落地顺序

1. 保留 `Engine/Shader/built-in`，新增工程目录 `Assets/Shaders`。
2. 给 Shader 引入稳定 GUID 和 `.meta`。
3. 将 `LoadShaderAsset()` 拆成逻辑资产加载与底层容器加载。
4. 修复 Shader 缓存键，改用完整逻辑 ID/GUID。
5. 给 `shader_compile` 增加 `-I`、`-D`、platform profile 参数。
6. 建立递归 include 依赖扫描。
7. 引入 `ShaderCookRequest` 和完整 `cookHash`。
8. 输出到 `DerivedData/Shaders/{platform}`。
9. 支持 Material 收集 Shader Variant。
10. 生成平台 Shader Manifest 和 Staging。
11. Android/iOS 只复制对应平台 Staging。
12. Shipping 模式禁止运行时源码 fallback。
13. 补全 uniform、texture、sampler 等反射数据。
14. 最后接入 DXIL/D3D12 和预编译 Metal Library。

最终推荐目录：

```text
Engine/Shader/built-in/          # 引擎 Shader 源码
Engine/Shader/include/           # 引擎公共 include
Project/Assets/Shaders/          # 工程 Shader 源码
Project/Assets/Shaders/Include/  # 工程公共 include

Project/Library/DerivedData/
└─ Shaders/{Platform}/           # 可删除、可重建的 Cook 缓存

Project/Build/Staging/{Platform}/
└─ Content/Shaders/              # 当前平台唯一发布产物
```

## 最终判断

现有 `Engine/Shader/built-in` 可以保留。真正需要调整的是，不要再把 `data_asset/Shader` 同时当作编译输出、缓存目录和最终发布目录。

工程级 Shader 应进入工程 `Assets/Shaders`，通过 GUID、平台 Cook、Variant 收集和 Manifest，与引擎 Shader 一起进入目标平台 Staging。

