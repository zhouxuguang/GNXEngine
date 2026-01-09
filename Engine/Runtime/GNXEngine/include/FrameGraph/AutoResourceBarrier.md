# 自动资源屏障系统使用指南

## 概述

自动资源屏障系统为 FrameGraph 提供了自动化的资源同步机制，自动处理纹理和缓冲区的状态转换和屏障插入，无需手动管理资源状态。

## 架构设计

### 核心组件

1. **ResourceBarrier.h**
   - 定义了资源访问类型（ResourceAccessType）
   - 定义了管线阶段（PipelineStage）
   - 定义了图像布局（ImageLayout）
   - 提供了状态转换辅助函数

2. **ResourceBarrierManager**
   - 管理资源的状态跟踪
   - 提供状态转换判断逻辑
   - 提供 Vulkan 参数转换函数

3. **FrameGraphTexture / FrameGraphBuffer**
   - 实现 preRead() 和 preWrite() 方法
   - 自动检测资源状态变化
   - 自动插入必要的屏障

## 使用方法

### 1. 在 Pass 中声明资源访问

在创建 Pass 时，使用 Builder 声明资源的读写操作：

```cpp
// 定义资源访问类型
enum MyTextureAccess {
    TEXTURE_READ = static_cast<uint32_t>(ResourceAccessType::ShaderResource),
    TEXTURE_WRITE = static_cast<uint32_t>(ResourceAccessType::ColorAttachment),
    TEXTURE_COMPUTE_READ = static_cast<uint32_t>(ResourceAccessType::ComputeShaderResource),
    TEXTURE_COMPUTE_WRITE = static_cast<uint32_t>(ResourceAccessType::StorageBufferWrite)
};

// 创建 Pass
fg.AddPass("GraphicsPass",
    [](FrameGraph::Builder& builder, auto& data) {
        // 声明读取纹理（作为着色器资源）
        data.inputTexture = builder.Read(srcTexture, TEXTURE_READ);
        
        // 声明写入纹理（作为颜色附件）
        data.outputTexture = builder.Write(dstTexture, TEXTURE_WRITE);
    },
    [](const auto& data, FrameGraphPassResources& resources, void* context) {
        // Pass 执行逻辑
        // context 是 VkCommandBuffer 指针
        // 系统会自动调用 preRead/preWrite 插入屏障
    }
);
```

### 2. 资源访问类型定义

系统支持以下资源访问类型：

**缓冲区访问：**
- `ResourceAccessType::VertexBuffer` - 顶点缓冲区
- `ResourceAccessType::IndexBuffer` - 索引缓冲区
- `ResourceAccessType::UniformBuffer` - Uniform 缓冲区
- `ResourceAccessType::StorageBufferRead` - 存储缓冲区读
- `ResourceAccessType::StorageBufferWrite` - 存储缓冲区写
- `ResourceAccessType::IndirectBuffer` - 间接绘制缓冲区
- `ResourceAccessType::TransferSrc` - 传输源
- `ResourceAccessType::TransferDst` - 传输目标

**纹理访问：**
- `ResourceAccessType::ShaderResource` - 着色器资源（纹理采样）
- `ResourceAccessType::ColorAttachment` - 颜色附件
- `ResourceAccessType::DepthStencilAttachment` - 深度模板附件
- `ResourceAccessType::DepthStencilReadOnly` - 深度模板只读
- `ResourceAccessType::ComputeShaderResource` - 计算着色器资源

### 3. 自动屏障逻辑

系统会自动执行以下操作：

1. **状态跟踪**
   - 记录每个资源的当前访问类型、管线阶段和图像布局

2. **状态比较**
   - 检测资源在新 Pass 中的使用方式是否与当前状态不同
   - 判断是否需要插入屏障

3. **屏障插入**
   - 计算必要的源/目标访问掩码
   - 计算必要的源/目标管线阶段
   - 调用 Vulkan 层的屏障 API

### 4. 传递 CommandBuffer 上下文

在执行 FrameGraph 时，需要传递 VkCommandBuffer 作为上下文：

```cpp
VkCommandBuffer commandBuffer = ...;

// 执行 FrameGraph
frameGraph.Execute(commandBuffer, allocator);
```

系统会自动在 Pass 执行前调用 preRead/preWrite，并传递 commandBuffer。

## 工作流程示例

### 场景：计算着色器 Pass 到 图形 Pass

```cpp
// 1. 计算着色器 Pass
fg.AddPass("ComputePass",
    [](FrameGraph::Builder& builder, auto& data) {
        // 读取纹理，写入结果
        data.texture = builder.Read(srcTexture, 
            static_cast<uint32_t>(ResourceAccessType::ComputeShaderResource));
        data.output = builder.Create<FrameGraphTexture>("Output", desc);
        builder.Write(data.output, 
            static_cast<uint32_t>(ResourceAccessType::StorageBufferWrite));
    },
    [](const auto& data, FrameGraphPassResources& resources, void* context) {
        // 计算着色器代码
    }
);

// 2. 图形 Pass
fg.AddPass("GraphicsPass",
    [](FrameGraph::Builder& builder, auto& data) {
        // 读取计算着色器的输出
        data.input = builder.Read(outputTexture, 
            static_cast<uint32_t>(ResourceAccessType::ShaderResource));
        data.colorTarget = builder.Write(backbuffer, 
            static_cast<uint32_t>(ResourceAccessType::ColorAttachment));
    },
    [](const auto& data, FrameGraphPassResources& resources, void* context) {
        // 渲染代码
    }
);

// 3. 编译和执行
fg.Compile();
fg.Execute(commandBuffer, allocator);
```

**系统自动执行的操作：**
- 在 ComputePass 前：插入屏障将纹理转换为 General 布局
- 在 ComputePass 后：纹理处于 ShaderWrite 状态
- 在 GraphicsPass 前：插入屏障将纹理转换为 ShaderReadOnly 布局

## 注意事项

1. **访问类型必须准确**
   - 如果声明为 ShaderResource 但在着色器中写入，会导致未定义行为
   - 系统不会验证实际使用是否与声明一致

2. **初始状态**
   - 新创建的资源初始状态为 Undefined
   - 第一次写入不需要屏障
   - 第一次读取需要屏障（除非资源是预初始化的）

3. **多个访问类型**
   - 可以使用位运算组合多个访问类型：
   ```cpp
   uint32_t flags = static_cast<uint32_t>(
       ResourceAccessType::ShaderResource | ResourceAccessType::DepthStencilReadOnly
   );
   ```

4. **性能考虑**
   - 系统只在必要时插入屏障
   - 避免不必要的状态转换可以提高性能
   - 尽量保持资源在合适的布局中

5. **跨队列同步**
   - 当前实现不支持跨队列同步
   - 如果需要跨队列同步，需要手动处理

## 扩展

### 添加新的资源类型

要支持新的资源类型，需要：

1. 创建资源类（继承相应的 RHI 资源）
2. 实现 Desc 结构体
3. 实现 create/destroy 方法
4. 实现 preRead/preWrite 方法

示例：

```cpp
class FrameGraphDepthTexture
{
public:
    struct Desc {
        RenderCore::Rect2D extent;
        RenderCore::TextureFormat format;
    };
    
    void create(const Desc& desc, void* allocator);
    void destroy(const Desc& desc, void* allocator);
    
    void preRead(const Desc& desc, uint32_t flags, void* context);
    void preWrite(const Desc& desc, uint32_t flags, void* context);
    
    static std::string toString(const Desc& desc);
    
    RenderCore::RCTexturePtr texture = nullptr;
};
```

## 故障排除

### 问题：屏障没有被插入

**可能原因：**
- 资源状态未初始化
- 访问类型与当前状态相同
- flags 参数不正确

**解决方法：**
- 检查 flags 是否正确映射到 ResourceAccessType
- 确认资源是否被正确初始化
- 添加调试日志查看状态转换

### 问题：性能下降

**可能原因：**
- 不必要的状态转换
- 频繁的屏障插入

**解决方法：**
- 合理组织 Pass 顺序，减少状态切换
- 使用相同的访问类型访问相同的资源
- 使用资源别名减少创建/销毁

## 参考资源

- Vulkan 规范：https://www.khronos.org/registry/vulkan/
- Frame Graph 论文：http://twvideo01.ubm-us.net/o1/vault/gdc2017/Presentations/Hammon_EA_Frostbite.pdf
