# GNXEngine 跨平台纹理打包系统分析与设计

## 结论

目前 GNXEngine 属于“运行时基本具备 ASTC 加载能力，但资产导入和打包链路还不能真正做到按平台生成、隔离和选择不同纹理格式”。

手工准备不同平台目录并由业务代码传入不同路径，理论上可以工作：

```text
data_asset/windows/pbr/albedo.texture   # BC7
data_asset/android/pbr/albedo.texture   # ASTC
```

但是，引擎目前还没有完整的平台纹理 Cook、平台缓存、自动路径解析和平台包裁剪机制。

## 当前能力判断

| 环节 | 当前状态 | 判断 |
|---|---|---|
| BC1/BC7 等压缩 | 导入器会根据源图通道生成 BC1/BC7 | 已支持 |
| ASTC 压缩器 | 有 `CompressASTC` 实现 | 组件存在，但未接入正式导入流程 |
| ASTC KTX 解析 | `ImageTextureUtil` 能识别 ASTC KTX1 | 已支持 |
| Vulkan/Metal ASTC 映射 | 多种 ASTC block size 都有映射 | 已支持 |
| 平台导入设置 | `compressFormat` 字段存在 | 表面支持，实际未生效 |
| 平台缓存隔离 | 只有单一 `.gnx/Cache` | 不支持 |
| 平台打包目录 | Android/iOS 直接复制公共 `data_asset` | 不支持 |
| 运行时自动选平台路径 | 调用方直接传文件路径 | 不支持 |
| 包体裁剪 | 整个资源目录复制进包 | 不支持 |

## 当前实现的关键问题

### 1. `compressFormat` 没有真正参与 Cook

虽然 `TextureImportSettings` 已经有 `compressFormat`：

- `Engine/Runtime/AssetProcess/include/TextureMetaFormat.h:63`

但真正生成 KTX 时，只按照源图片像素格式调用 `CreateKTXFormat()`：

- `Engine/Runtime/AssetProcess/source/TextureImporter.cpp:557`

当前映射大致是：

- RGBA8 → BC7 UNORM
- SRGBA8 → BC7 sRGB
- RGB8 → BC1
- HDR RGB32 → BC6H

`textureImportSettings.compressFormat` 没有参与格式选择。因此，即使在 meta 中把格式配置成 ASTC，导入结果仍然会是 BC 格式。

ASTC 压缩器虽然存在，但没有进入 `CompressTextureInner()` 分支。模型打包器也明确固定生成 BC7：

- `Engine/Runtime/AssetProcess/source/ModelAssetPackager.cpp:336`

### 2. 运行时与资产构建能力不一致

`ImageTextureUtil` 已经能把 KTX1 的 ASTC GL format 转换为引擎格式：

- `Engine/Runtime/RenderSystem/source/ImageTextureUtil.cpp:342`

随后可以从 `.texture` 容器中解出 KTX 并创建 GPU 纹理：

- `Engine/Runtime/RenderSystem/source/ImageTextureUtil.cpp:807`
- `Engine/Runtime/RenderSystem/source/ImageTextureUtil.cpp:849`

因此，只要生成正确的 ASTC KTX1 `.texture`，Vulkan/Metal 加载链路基本已经具备条件。

不过，另一套 `TextureAsset::ParseMeta()` 格式转换没有 ASTC 分支：

- `Engine/Runtime/AssetManager/source/TextureAsset.cpp:269`

两套格式解析实现不一致，后续必须统一，否则 `TextureAsset::GetFormat()` 可能得到 Invalid。

### 3. 公共产物目录会造成重复与覆盖

Android、iOS 当前都围绕公共 `data_asset` 打包：

- Android terrain demo：`demo/terrain/android/app/build.gradle:40`
- iOS terrain demo：`demo/terrain/CMakeLists.txt:34`

这会导致：

- 没有 Windows、Android、iOS 的独立资源产物。
- 如果把 BC 和 ASTC 都放进去，移动包会同时包含两份纹理。
- 如果保持同名文件，就只能由最后一次构建覆盖。
- 打包结果会受工作目录中的残留产物影响。
- 无法可靠进行增量 Cook 和包体统计。

### 4. 运行时路径和缓存键存在冲突

`LoadTextureInternal()` 直接使用调用方传入的路径：

- `Engine/Runtime/AssetManager/source/AssetManager.cpp:155`

纹理缓存 key 只取文件名，不包含目录：

- `Engine/Runtime/AssetManager/source/AssetManager.cpp:170`

因此以下两个资源会发生缓存冲突：

```text
characters/hero/albedo.texture
environment/forest/albedo.texture
```

平台目录不能彻底解决该问题，因为最终 key 仍然都是 `albedo`。

## 推荐系统分层

```text
源资产 Assets
    ↓ Import / Cook
平台派生缓存 DerivedData
    ↓ Stage
平台暂存目录 Staging
    ↓ Package
APK / IPA / Windows 发布目录
```

## 1. 源资产层

源文件保持一份：

```text
Project/
├─ Assets/
│  ├─ Characters/Hero/Albedo.png
│  ├─ Characters/Hero/Normal.png
│  └─ Environment/Sky.hdr
└─ Library/
```

`.meta` 记录稳定 GUID、通用设置和平台覆盖：

```yaml
guid: "95c4..."

texture:
  semantic: Albedo
  colorSpace: sRGB
  mipmaps: true
  maxSize: 4096
  quality: 80

platformOverrides:
  Windows:
    format: BC7
  Android:
    format: ASTC_6x6
    maxSize: 2048
  iOS:
    format: ASTC_6x6
```

资产引用应保存 GUID 或逻辑路径，不能保存构建机绝对路径，也不应保存 `{hash}.texture` 的物理路径。

## 2. 平台派生缓存

```text
Library/DerivedData/
├─ Windows-DX12/
│  └─ Texture/{cookHash}.texture
├─ Android-Vulkan/
│  └─ Texture/{cookHash}.texture
└─ iOS-Metal/
   └─ Texture/{cookHash}.texture
```

`cookHash` 必须由以下内容共同计算：

```text
源文件内容 hash
+ importerVersion
+ compressorVersion
+ TargetPlatform
+ GPU format
+ ASTC block size
+ colorSpace
+ mipmap 设置
+ maxSize
+ normal-map 处理设置
+ compression quality
```

当前只根据源文件 hash 确定输出路径是不够的：

- `Engine/Runtime/AssetProcess/source/TextureImporter.cpp:813`

同一源图的 BC7、ASTC 6×6 和 ASTC 8×8 必须得到不同的缓存键。

## 3. 平台暂存目录

Cook 完成后，通过依赖收集生成干净的目标平台目录：

```text
Build/Staging/
├─ Windows/
│  ├─ AssetManifest.bin
│  └─ Content/Characters/Hero/Albedo.texture
├─ Android/
│  ├─ AssetManifest.bin
│  └─ Content/Characters/Hero/Albedo.texture
└─ iOS/
   ├─ AssetManifest.bin
   └─ Content/Characters/Hero/Albedo.texture
```

三个平台的逻辑路径相同，但文件内容不同。游戏代码无需拼接 `windows/`、`android/` 路径。应用包里只放目标平台的 Staging 内容，自然不会同时出现 BC 和 ASTC。

发布包应该是确定性的，不应依靠运行时搜索和回退隐藏打包错误。

## 4. AssetManifest

每个平台生成独立 manifest：

```json
{
  "platform": "Android-Vulkan",
  "buildVersion": 7,
  "assets": {
    "95c4...": {
      "type": "Texture2D",
      "path": "Content/Characters/Hero/Albedo.texture",
      "contentHash": "...",
      "format": "ASTC_6x6_SRGB",
      "dependencies": []
    }
  }
}
```

运行时流程：

```text
GUID / 逻辑路径
    ↓
AssetManifest 查询
    ↓
目标平台相对路径
    ↓
AssetProvider 读取
    ↓
解析 .texture / KTX
    ↓
验证 GPU 格式支持
    ↓
创建 GPU Texture
```

## 平台纹理策略

| 纹理用途 | Windows | Android/iOS |
|---|---|---|
| Albedo/BaseColor | BC7 sRGB | ASTC 6×6 sRGB |
| 带透明颜色 | BC7 sRGB | ASTC 4×4 或 6×6 sRGB |
| Normal | BC5 UNORM | ASTC 6×6 UNORM |
| ORM/遮罩 | BC7/BC4/BC5 UNORM | ASTC 6×6 或 8×8 UNORM |
| UI | BC7 或 RGBA8 | ASTC 4×4，必要时 RGBA8 |
| HDR 环境 | BC6H | 暂时 RGBA16F，之后单独设计 ASTC HDR |
| 低端移动设备 | 不适用 | ETC2 fallback |

移动端不能简单规定所有资源都是 ASTC：

- Android 设备需要检查 ASTC 支持，或者明确最低设备标准。
- 如需覆盖不支持 ASTC 的设备，应建立 `Android-ETC2` profile。
- iOS 较新的设备通常适合 ASTC，但仍应由设备能力表决定。
- ASTC LDR 和 ASTC HDR 应视为不同能力。

## 建议增加的核心对象

```cpp
enum class TargetPlatform
{
    WindowsDX12,
    AndroidVulkanASTC,
    AndroidVulkanETC2,
    IOSMetal
};

struct TextureCookSettings
{
    TargetPlatform platform;
    TextureFormat format;
    uint8_t blockWidth;
    uint8_t blockHeight;
    bool srgb;
    bool generateMipmaps;
    uint32_t maxSize;
    uint32_t quality;
};

class TextureFormatPolicy
{
public:
    TextureCookSettings Resolve(
        TargetPlatform platform,
        TextureSemantic semantic,
        const TextureImportSettings& commonSettings,
        const PlatformOverride& overrideSettings);
};

class AssetCooker;
class AssetManifestBuilder;
class AssetStager;
class AssetPathResolver;
```

应由 `TextureFormatPolicy` 统一负责“平台 + 纹理语义 → 最终格式”，不要把平台判断散落在 `TextureImporter`、模型导入器和 CMake 中。

## 运行时路径设计

建议提供统一接口：

```cpp
TextureHandle LoadTexture(AssetId id);
TextureHandle LoadTexture("Characters/Hero/Albedo");
```

内部由 `AssetPathResolver` 解析，业务层不接触平台目录。

短期过渡方案可以先支持：

```cpp
AssetManager::Initialize(assetRoot, TargetPlatform::AndroidVulkanASTC);
```

根路径指向当前平台 Staging：

```text
Windows: Build/Staging/Windows
Android: APK assets 根目录
iOS: App Bundle Resources 根目录
```

所有平台仍加载相同逻辑路径：

```cpp
LoadTexture("Content/Characters/Hero/Albedo");
```

## 推荐实施顺序

1. 修正 `TextureImporter`，让最终格式真正来自 `TextureCookSettings`。
2. 把 ASTC 压缩器接入 mip 生成和 KTX1 写入流程。
3. 统一 `TextureAsset` 与 `ImageTextureUtil` 的格式转换代码。
4. 引入 `TargetPlatform` 和平台纹理策略。
5. 将缓存改为 `DerivedData/{platform}/{cookHash}`。
6. 引入稳定 GUID 和平台 `AssetManifest`。
7. 修复缓存 key，至少使用规范化完整逻辑路径，最终使用 GUID。
8. 新增干净的 `Staging/{platform}` 阶段。
9. Android/iOS 构建只复制对应 Staging 目录。
10. 增加自动测试：同一 PNG 分别 Cook 为 BC7、ASTC，并验证 KTX format、mip 大小和运行时上传。

## 最终判断

GNXEngine 的底层已经具备大约一半的跨平台压缩纹理基础，但目前还不能称为“支持不同平台不同纹理格式的打包系统”。

最合适的改造路线不是简单增加平台文件夹，而是补齐完整链路：

```text
Platform Profile
    → Cook Cache
    → Asset Manifest
    → Platform Staging
    → Package
```

