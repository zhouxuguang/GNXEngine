# RHI 抽象资源屏障系统

## 概述

自动资源屏障系统已完全集成到 RHI（Render Hardware Interface）抽象层中，通过虚函数接口实现平台无关的资源同步。

## 架构设计

### 核心组件

1. **RCTexture.h** - 纹理资源抽象
   - `ResourceAccess` 枚举：定义资源访问类型
   - `ResourcePipelineStage` 枚举：定义管线阶段
   - `ResourceLayout` 枚举：定义资源布局
   - `ResourceState` 结构：资源状态
   - `GetState()` / `SetState()`：状态管理
   - `PreReadBarrier()` / `PreWriteBarrier()`：屏障插入接口

2. **ComputeBuffer.h** - 缓冲区资源抽象
   - 相同的状态和屏障接口
   - 专门用于缓冲区资源

3. **FrameGraphTexture.cpp / FrameGraphBuffer.cpp** - 使用 RHI 接口
   - 通过 `texture->PreReadBarrier()` 调用
   - 不依赖平台特定的 API

4. **Vulkan 实现**
   - `VKTextureBase`：实现纹理屏障逻辑
   - `VKComputeBuffer`：实现缓冲区屏障逻辑
   - 使用 Vulkan API 插入实际的屏障

## 使用方法

### 1. 定义资源访问类型

```cpp
// 使用 RHI 抽象的访问类型
enum MyTextureAccess {
    TEXTURE_READ = static_cast<uint32_t>(RenderCore::ResourceAccess::ShaderResource),
    TEXTURE_WRITE = static_cast<uint32_t>(RenderCore::ResourceAccess::ColorAttachment),
    TEXTURE_COMPUTE_READ = static_cast<uint32_t>(RenderCore::ResourceAccess::ComputeShaderResource),
    TEXTURE_COMPUTE_WRITE = static_cast<uint32_t>(RenderCore::ResourceAccess::StorageBufferWrite)
};
```

### 2. 在 Pass 中使用

```cpp
fg.AddPass("MyPass",
    [](FrameGraph::Builder& builder, auto& data) {
        // 读取纹理
        data.input = builder.Read(srcTexture, TEXTURE_READ);
        
        // 写入纹理
        data.output = builder.Write(dstTexture, TEXTURE_WRITE);
    },
    [](const auto& data, FrameGraphPassResources& resources, void* context) {
        // 系统自动调用 RHI 抽象接口插入屏障
        // context 是平台特定的命令缓冲区（VkCommandBuffer/MtlCommandBuffer 等）
    }
);
```

### 3. 执行 FrameGraph

```cpp
// 传递平台特定的命令缓冲区
VkCommandBuffer commandBuffer = ...;
frameGraph.Execute(commandBuffer, allocator);
```

## RHI 抽象接口

### 纹理屏障接口

```cpp
class RCTexture {
public:
    // 获取当前状态
    virtual ResourceState GetState() const = 0;
    
    // 设置状态
    virtual void SetState(const ResourceState& state) = 0;
    
    // 读取前插入屏障
    virtual void PreReadBarrier(void* commandBuffer, 
                             ResourceAccess access, 
                             ResourcePipelineStage stage, 
                             ResourceLayout layout) = 0;
    
    // 写入前插入屏障
    virtual void PreWriteBarrier(void* commandBuffer, 
                             ResourceAccess access, 
                             ResourcePipelineStage stage, 
                             ResourceLayout layout) = 0;
};
```

### 缓冲区屏障接口

```cpp
class ComputeBuffer {
public:
    // 获取当前状态
    virtual ResourceState GetState() const = 0;
    
    // 设置状态
    virtual void SetState(const ResourceState& state) = 0;
    
    // 读取前插入屏障
    virtual void PreReadBarrier(void* commandBuffer, 
                             ResourceAccess access, 
                             ResourcePipelineStage stage) = 0;
    
    // 写入前插入屏障
    virtual void PreWriteBarrier(void* commandBuffer, 
                             ResourceAccess access, 
                             ResourcePipelineStage stage) = 0;
};
```

## 平台实现

### Vulkan 实现

#### VKTextureBase 实现

```cpp
// 获取状态
ResourceState VKTextureBase::GetState() const
{
    return mResourceState;
}

// 设置状态
void VKTextureBase::SetState(const ResourceState& state)
{
    mResourceState = state;
    mCurrentLayout = GetVulkanImageLayout(state.layout);
}

// 读取屏障
void VKTextureBase::PreReadBarrier(void* commandBuffer, 
                                   ResourceAccess access,
                                   ResourcePipelineStage stage, 
                                   ResourceLayout layout)
{
    VkCommandBuffer cmdBuffer = static_cast<VkCommandBuffer>(commandBuffer);
    
    // 计算源和目标访问掩码
    uint32_t srcAccessMask = GetVulkanAccessMask(mResourceState.access);
    uint32_t dstAccessMask = GetVulkanAccessMask(access);
    
    // 计算源和目标管线阶段
    uint32_t srcStageMask = GetVulkanPipelineStageMask(mResourceState.stage);
    uint32_t dstStageMask = GetVulkanPipelineStageMask(stage);
    
    // 计算图像布局
    VkImageLayout oldLayout = GetVulkanImageLayout(mResourceState.layout);
    VkImageLayout newLayout = GetVulkanImageLayout(layout);
    
    // 创建图像子资源范围
    VkImageSubresourceRange subresourceRange = {};
    // ... 设置 subresourceRange ...
    
    // 插入 Vulkan 屏障
    VulkanBufferUtil::InsertImageMemoryBarrier(
        cmdBuffer,
        mImage,
        srcAccessMask,
        dstAccessMask,
        oldLayout,
        newLayout,
        srcStageMask,
        dstStageMask,
        subresourceRange
    );
    
    // 更新状态
    mCurrentLayout = newLayout;
}
```

#### VKComputeBuffer 实现

```cpp
// 读取屏障
void VKComputeBuffer::PreReadBarrier(void* commandBuffer, 
                                   ResourceAccess access,
                                   ResourcePipelineStage stage)
{
    VkCommandBuffer cmdBuffer = static_cast<VkCommandBuffer>(commandBuffer);
    
    // 计算源和目标访问掩码
    uint32_t srcAccessMask = GetVulkanAccessMask(mResourceState.access);
    uint32_t dstAccessMask = GetVulkanAccessMask(access);
    
    // 计算源和目标管线阶段
    uint32_t srcStageMask = GetVulkanPipelineStageMask(mResourceState.stage);
    uint32_t dstStageMask = GetVulkanPipelineStageMask(stage);
    
    // 插入 Vulkan 缓冲区屏障
    VulkanBufferUtil::InsertBufferMemoryBarrier(
        cmdBuffer,
        mBuffer,
        0,          // offset
        mBufferLength, // size
        srcAccessMask,
        dstAccessMask,
        srcStageMask,
        dstStageMask
    );
    
    // 更新状态
    mResourceState.access = access;
    mResourceState.stage = stage;
    mResourceState.initialized = true;
}
```

### Metal 实现（示例）

```cpp
class MTLTextureBase : public RCTexture {
public:
    void PreReadBarrier(void* commandBuffer, 
                     ResourceAccess access,
                     ResourcePipelineStage stage,
                     ResourceLayout layout) override
    {
        id<MTLCommandBuffer> cmdBuffer = static_cast<id<MTLCommandBuffer>>(commandBuffer);
        
        // 转换为 Metal 的编码
        MTLBarrierScope barrierScope = ConvertToMTLScope(access, stage);
        
        // 插入 Metal 屏障
        [cmdBuffer textureBarrier];
    }
};
```

## 优势

### 1. 平台无关性
- FrameGraph 层不需要知道具体平台
- 使用统一的 RHI 抽象接口
- 易于添加新的平台支持

### 2. 可扩展性
- 添加新的平台只需实现 RHI 接口
- 不需要修改 FrameGraph 层代码
- 可以针对不同平台优化

### 3. 类型安全
- 编译时检查接口实现
- 避免平台特定的代码泄漏

### 4. 维护性
- 平台特定的代码集中在 RHI 层
- FrameGraph 代码简洁清晰
- 易于调试和测试

## 转换函数

每个平台实现需要提供以下转换函数：

### 纹理转换
```cpp
// ResourceAccess -> VkAccessFlags
static uint32_t GetVulkanAccessMask(ResourceAccess access);

// ResourcePipelineStage -> VkPipelineStageFlags
static uint32_t GetVulkanPipelineStageMask(ResourcePipelineStage stage);

// ResourceLayout -> VkImageLayout
static VkImageLayout GetVulkanImageLayout(ResourceLayout layout);
```

### 缓冲区转换
```cpp
// ResourceAccess -> VkAccessFlags
static uint32_t GetVulkanAccessMask(ResourceAccess access);

// ResourcePipelineStage -> VkPipelineStageFlags
static uint32_t GetVulkanPipelineStageMask(ResourcePipelineStage stage);
```

## 工作流程

1. **Pass 声明**
   ```cpp
   builder.Read(texture, RESOURCE_ACCESS_FLAGS);
   builder.Write(texture, RESOURCE_ACCESS_FLAGS);
   ```

2. **编译阶段**
   - FrameGraph 构建资源依赖图
   - 确定资源使用顺序

3. **执行阶段**
   - 对于每个资源：
     a. 调用 `preRead()` 或 `preWrite()`
     b. 这些方法调用 RHI 抽象接口 `texture->PreReadBarrier()`
     c. RHI 实现调用平台特定的 API 插入屏障
     d. 更新资源状态

4. **屏障插入**
   - 只在状态变化时插入
   - 自动优化（避免不必要的屏障）
   - 确保正确的同步

## 故障排除

### 问题：屏障没有被插入

**检查清单：**
1. flags 参数是否正确映射到 ResourceAccess？
2. 资源状态是否正确初始化？
3. RHI 实现的转换函数是否正确？

**调试方法：**
```cpp
// 在 RHI 实现中添加日志
void VKTextureBase::PreReadBarrier(...) {
    LOG_DEBUG("PreReadBarrier: old access=%d, new access=%d", 
              mResourceState.access, access);
    // ... 实现代码 ...
}
```

### 问题：编译错误

**常见问题：**
- 未实现 RHI 虚函数
- 转换函数签名不匹配
- 缺少必要的头文件

**解决方法：**
- 确保所有 RHI 类都实现了虚函数
- 检查平台特定的头文件包含
- 使用正确的类型转换

## 性能优化

1. **减少屏障**
   - 合理组织 Pass 顺序
   - 使用相同的访问类型访问相同资源
   - 避免频繁的状态切换

2. **批量操作**
   - 系统会自动合并相同状态的屏障
   - 一次性处理多个资源

3. **异步执行**
   - 支持异步计算队列
   - 在 EnableAsyncCompute(true) 的 Pass 中使用

## 参考资源

- Vulkan 规范：https://www.khronos.org/registry/vulkan/
- Metal 编程指南：https://developer.apple.com/metal/
- Frame Graph 论文：http://twvideo01.ubm-us.net/o1/vault/gdc2017/Presentations/Hammon_EA_Frostbite.pdf
