# GNXEngine

轻量级跨平台游戏引擎，使用 C++17 开发，CMake 构建，目前支持 macOS 和 Windows。

## 特性

- **跨平台 RHI**：图形 API 兼容 Metal 和 Vulkan，预留其它图形 API 接入能力
- **自主基础设施**：多线程、线程池、时间、日期、日志、字符串等基础功能
- **自主数学库**：向量、矩阵、四元数等 3D 数学运算
- **资源导入**：使用 Assimp 导入静态网格、蒙皮网格及动画资源；支持 PNG/JPEG/TGA/KTX1 纹理格式
- **PBR/IBL 渲染**：基于物理的渲染与基于图像的光照
- **动画系统**：动画姿态插值、CPU/GPU 蒙皮，优化了局部到全局变换的转换
- **HLSL Shader 管线**：DXC → SPIR-V → Spirv-Cross → 各后端 Shader 语言
- **Mesh Shader**：支持 Task + Mesh Shader 管线（Metal / Vulkan）
- **Entity-Component 架构**：传统的 Entity-Component 模式构造上层业务

## 架构

```
                              ◈  GNXEngine  ◈

  ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
  ┃                                                        ┃
  ┃              ▌ Editor  ·  Qt 编辑器                     ┃
  ┃                            │                            ┃
  ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━━━┛
                                 ▼
  ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
  ┃                                                        ┃
  ┃    ▌ GNXEngine  ·  引擎核心  (事件 · 输入 · 序列化)    ┃
  ┃                                                        ┃
  ┗━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━┛
                  ▼                        ▼
  ╭─────────────────────────╮    ╭─────────────────────────╮
  │   ▌ RenderSystem        │    │   ▌ AssetProcess        │
  │     场景 · 相机 · 光照   │◄──►│     模型导入 · IBL      │
  │     帧图 · 后处理        │    │     纹理压缩 · 转换      │
  ╰────────────┬─────────────╯    ╰─────┬──────┬───────────╯
               │                        │      │
        ┌──────┴──────┐                 │      ▼
        ▼             ▼                 │  ╭─────────────────────╮
  ╭─────────────╮ ╭───────────────╮     │  │  ▌ ImageCodec       │
  │ ▌RenderCore │ │ ▌ShaderCompiler│     │  │    PNG · JPEG · KTX │
  │  RHI 抽象层 │ │  HLSL ┈► SPIR-V│     │  ╰─────────┬───────────╯
  │             │ │        ┈► MSL  │     │            │
  │ ┌─────────┐ │ │        ┈► SPIR-V│     │            │
  │ │◈ Metal  │ │ ╰───────┬───────╯     │            │
  │ ├─────────┤ │         │             │            │
  │ │◈ Vulkan │ │◄────────┘             │            │
  │ └─────────┘ │                        │            │
  ╰──────┬──────╯                        │            │
         │              ╭─────────────────╯            │
         │              ▼                              │
         │     ╭─────────────────────╮                 │
         │     │  ▌ AssetManager     │◄────────────────┘
         │     │    资源加载 · 缓存   │
         │     ╰──────────┬──────────╯
         │                │
         │                ▼
         │          ╭─────────────╮
         │          │ ▌ MathUtil  │
         │          │ 向量 · 矩阵 │
         │          ╰──────┬──────╯
         │                 │
         └──────┬──────────┘
                ▼
          ╭─────────────╮  ╭──────────────╮
          │ ▌ BaseLib   │  │ ▌ Allocator  │
          │ 线程 · 日志  │  │   内存分配    │
          ╰─────────────╯  ╰──────┬───────╯
                                   │
                                   ▼
                             ╭─────────────╮
                             │ ▌ BaseLib   │
                             ╰─────────────╯
```

### 核心模块说明

| 模块 | 说明 | 依赖 |
|------|------|------|
| **GNXEngine** | 引擎核心库，事件系统、输入、序列化 | RenderSystem, AssetProcess, AssetManager, Allocator |
| **RenderSystem** | 渲染系统上层，场景、相机、光照、帧图、后处理 | RenderCore, ShaderCompiler |
| **AssetProcess** | 资源处理，模型导入(Assimp)、IBL、纹理压缩转换 | RenderSystem, AssetManager, ImageCodec |
| **AssetManager** | 资源管理器，资源加载、缓存、生命周期 | ImageCodec |
| **RenderCore** | RHI 抽象层，GPU 资源与操作接口，Metal/Vulkan 双后端 | BaseLib |
| **ShaderCompiler** | Shader 编译管线，HLSL → SPIR-V → MSL/SPIR-V | RenderCore |
| **ImageCodec** | 图像编解码，PNG/JPEG/TGA/KTX | MathUtil |
| **MathUtil** | 3D 数学库，向量、矩阵、四元数 | BaseLib |
| **Allocator** | 内存分配器 | BaseLib |
| **BaseLib** | 基础库，多线程、线程池、日志、时间、字符串 | — |

## 编译

### 前置要求

#### 必装工具

| 工具 | 版本要求 | 说明 |
|------|---------|------|
| **CMake** | 3.17+ | 构建系统 |
| **C++17 编译器** | — | macOS: Xcode Command Line Tools；Windows: MSVC 2019+ |
| **Intel ISPC** | 1.x+ | 纹理压缩编译所需，需安装并设置环境变量 |

#### ISPC 安装

1. 从 [Intel ISPC Releases](https://github.com/ispc/ispc/releases) 下载对应平台版本
2. 解压到本地目录
3. 设置环境变量，CMake 会通过以下路径查找 ISPC：

**macOS / Linux：**

```bash
# 方式一：设置 ISPC_HOME 环境变量
export ISPC_HOME=/path/to/ispc

# 方式二：将 ispc 放入 PATH
export PATH=/path/to/ispc/bin:$PATH

# 方式三：安装到 /usr/local/bin（CMake 默认搜索路径）
cp /path/to/ispc/bin/ispc /usr/local/bin/
```

建议将环境变量写入 `~/.zshrc` 或 `~/.bashrc` 使其永久生效：

```bash
echo 'export ISPC_HOME=/path/to/ispc' >> ~/.zshrc
```

**Windows：**

1. 下载 `ispc-vX.X.X-windows.zip` 并解压（如 `C:\ispc`）
2. 设置系统环境变量 `ISPC_HOME` 指向解压目录（如 `C:\ispc`）

#### 可选依赖（仅编辑器）

| 工具 | 说明 |
|------|------|
| **Qt6** (或 Qt5) | 编辑器所需，仅需 Widgets 模块，编译时加 `-DENABLE_EDITOR=ON` |

#### 已内嵌的依赖

以下依赖已在 `ThirdParty/` 目录中，无需单独安装：

DXC (Shader 编译), Assimp (模型导入), SPIRV-Reflect, TBB, GLFW, KTX, nlohmann_json, nanopb, yaml-cpp, mimalloc, meshoptimizer, zlib, miniz, pvrtc, Vulkan Headers

### 编译步骤

```bash
# 生成构建文件
cmake -B build

# 编译
cmake --build build --config Debug
```

### 编译选项

通过 `-D` 参数控制可选模块的编译：

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `ENABLE_EDITOR` | 编译编辑器（需安装 Qt） | OFF |
| `ENABLE_TESTING` | 编译单元测试 | OFF |

示例：

```bash
# 编译全部模块（含编辑器和测试）
cmake -B build -DENABLE_EDITOR=ON -DENABLE_TESTING=ON

# 仅编译编辑器
cmake -B build -DENABLE_EDITOR=ON

# 编译并运行单元测试
cmake -B build -DENABLE_TESTING=ON
cmake --build build
cd build && ctest
```

### Demo

编译后在 `build/Debug/` 目录下可直接运行各 Demo：

- `pbr` — PBR 渲染示例
- `terrain` — 地形渲染
- `meshshader` — Mesh Shader 示例
- `ssao` — 屏幕空间环境光遮蔽
- `lumen` / `nanite` — 实验性功能
