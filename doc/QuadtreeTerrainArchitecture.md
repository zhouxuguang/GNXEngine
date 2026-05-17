# GNXEngine 四叉树地形系统：从架构到渲染，一个完整的 GPU-Driven 地形故事

## 写在前面

从最早的一张静态网格地形，到现在的三路渲染管线（传统 VS+PS Instanced、Compute Shader 剔除 + Indirect Draw、Mesh Shader TS+MS），经历了不少迭代。

## 一、整体架构：分层的设计

整个地形模块分成四个层次，每一层各司其职：

```
┌─────────────────────────────────────┐
│   TerrainFrameWork (Demo层)         │
│   负责创建组件、每帧更新相机、触发渲染    │
├─────────────────────────────────────┤
│   TerrainComponent (渲染管理层)       │
│   封装地形整体渲染流程：选择渲染路径、     │
│   绑定资源、管理UBO三缓冲              │
├─────────────────────────────────────┤
│   QuadTreeTerrain (核心数据层)         │
│   四叉树维护、LOD选择、裂缝修复、       │
│   静态索引池、PatchMeta生成             │
├─────────────────────────────────────┤
│   TerrainCullPass (GPU剔除层)         │
│   Compute Shader做视锥体剔除，          │
│   输出Indirect Draw Command            │
└─────────────────────────────────────┘
```

另外还有多个 **Shader 文件**和 **TerrainGenerator 工具类**。

## 二、核心数据结构：从高度图开始

### 2.1 高度图 → 地形网格

地形的源头是一张**高度图**（GRAY8/GRAY16 的 PNG），或者一段**程序化正弦波噪声**。初始化时，引擎先取高度图的尺寸，向上取整到 `2ⁿ + 1`。如果高度图是 1025×1025，那网格大小就是 1025（因为 1024 = 2¹⁰），顶点数就是 1025² ≈ 100 万个顶点。

每个顶点的数据是这样拼的：

```
VertexData (SoA布局):
┌─────────────────────────────────────┐
│ Position (Vector3f)                  │
│ Tangent  (Vector4f)                  │
│ Normal   (Vector3f)                  │
│ TexCoord (Vector2f)                  │
└─────────────────────────────────────┘
```

法线是通过**中心差分**从高度图算出来的，不是从 shader 里实时算。切线也是同理。这样在传统 VS+PS 渲染路径中，shader 可以直接用已经算好的数据。

### 2.2 四叉树节点

```cpp
struct Node {
    uint32_t x, z;               // 网格坐标中的左上角
    uint32_t size;               // 网格单元尺寸（2的幂）
    uint32_t level;              // LOD层级：越大越粗，0=最细
    float maxGeoError;           // 最大几何误差
    mathutil::AxisAlignedBoxf bounds; // 世界空间包围盒
    Node* parent;                // 父节点指针
    std::unique_ptr<Node> children[4]; // [0]=NW, [1]=NE, [2]=SW, [3]=SE
};
```

`level` 的设计跟一般的 LOD 是反的——`mMaxLevel` 是最粗（根节点），`0` 是最细（叶节点最小尺寸）。这个设计是为配合高度图格网的索引计算。

`size` 存储的是**网格单元数**（cells），不是顶点数。如果一个节点覆盖了 1024 个单元，它的 size 就是 1024。

### 2.3 PatchMeta：GPU 端的叶节点身份证

每个叶节点（最终渲染的 patch）会被打包成一个 `PatchMeta` 结构体，上传到 SSBO 供 GPU 读取。所有地形 shader 都需要保持 C++ 和 HLSL 两端结构体定义完全一致：

```hlsl
struct PatchMeta {
    float worldX, worldZ;       // patch 左上角世界坐标
    float worldSize;            // patch 的世界空间尺寸
    float minHeight;            // AABB 最小 Y
    uint  gridX, gridZ;         // 网格起始坐标
    uint  gridSize;             // 网格单元尺寸
    uint  level;                // LOD 层级
    uint  neighborFlags;        // 裂缝信息：哪个方向的邻居更粗
    // padding to 48 bytes (16-byte alignment for StructuredBuffer)
};
```

`neighborFlags` 这个字段是后来做 Mesh Shader 裂缝修复才加的。

## 三、LOD 选择：基于距离的细分与合并

### 3.1 我应该继续细分吗？

每次更新，从根节点开始递归，对每个节点调用 `ShouldSubdivide`：

```cpp
bool ShouldSubdivide(const Node& node, const Vector3f& cameraPos) const {
    if (node.level == 0) return false;  // 已经最细了
    double nodeWorldSize = (double)node.size * worldStep;
    double dx = cameraPos.x - node.bounds.center.x;
    double dz = cameraPos.z - node.bounds.center.z;
    double dy = cameraPos.y - node.bounds.center.y;
    double distance = sqrt(dx * dx + dy * dy + dz * dz);
    return (distance / nodeWorldSize) < mLODDistanceFactor;
}
```

**相机离节点的距离除以节点的世界尺寸**，小于一个阈值就细分。大节点在远处就开始细分，小节点要很近才细分。

### 3.2 合并判断

`ShouldMerge` 跟 `ShouldSubdivide` 相反，但加了一个**迟滞阈值**（`mLODDistanceFactor * 1.49`），防止相机在边界附近来回晃动时节点频繁拆分/合并。

### 3.3 算法流程

```
Update(cameraPos)
  └→ UpdateNode(根节点, cameraPos)
       ├→ 距离近 → 如果已经是叶子则 Subdivide(创建4个子节点)
       │           否则递归处理各个子节点
       ├→ 距离远且超过合并阈值 → 销毁子节点（合并）
       └→ 邻居约束检查：如果邻居比自己细超过1级，强制细分
```

等树更新完了，再 `CollectLeaves` 把所有叶节点收到一个 flat 数组里。

## 四、裂缝修复：三角形扇方案

### 4.1 裂缝怎么来的？

相邻两个 patch 的 LOD 级不同，它们共用一条边。细的那边有 9 个顶点（8 个单元），粗的那边只有 5 个顶点（4 个单元）。粗 patch 边上那些"多余"的顶点无法对齐，形成裂缝。

### 4.2 基本策略：邻居层级差不超过 1

前提条件：相邻叶节点的层级差不超过 1。`EnforceNeighborConstraint` 做的事就是反复遍历所有叶节点，找有没有邻居比自己细超过 1 级的，有就强制细分。

### 4.3 传统路径：三角形扇形（Triangle Fan）

通过**改变索引缓冲区的排列**来实现。每个 patch 的边上，如果邻居更粗，这条边上的顶点就不连到边上的相邻顶点，而是连到邻居的粗顶点上，形成一个扇形，把裂缝补上。

实现方式是在 `BuildStaticIndexPool` 阶段预计算——对每种步长 × 每种邻居排列（16 种），生成对应的索引序列。运行时查表 O(1)，零开销。

### 4.4 Mesh Shader 路径：顶点位置修正

Mesh Shader 路径不走索引缓冲区，所以用顶点修正：

```hlsl
uint nf = meta.neighborFlags;
if (col == 0 && (row & 1) && (nf & 1u)) { // 左邻居更粗
    height = (h0 + h1) * 0.5; // 取上下两个粗顶点的平均高度
}
// 其他三个方向同理
```

"中间顶点"取相邻两个粗顶点的**高度平均值**，自然消除裂缝。

## 五、渲染管线：三条路径

### 5.1 路径一：Mesh Shader（最新最高优先级）

```
TS ([numthreads(32,1,1)])
 ├→ 读 gPatchMeta[]
 ├→ AABBInFrustum 视锥体剔除
 ├→ WavePrefixCountBits 紧凑打包
 └→ DispatchMesh(visibleCount, 1, 1)
     │
     ▼
MS ([numthreads(32,1,1)], outputtopology="triangle")
 ├→ SetMeshOutputCounts(81, 128) — 81个顶点, 128个三角形
 ├→ Vertex: 每个线程处理 ceil(81/32)=3个顶点
 │   ├→ UV → 世界坐标映射
 │   ├→ 高度图采样
 │   ├→ 裂缝修复（neighborFlags）
 │   ├→ 有限差分法算法线/切线
 │   └→ Clip变换 + prevVP（运动向量）
 ├→ Index: 每个线程处理 ceil(64/32)=2个三角形
 └→ 输出 uint3 triangles[128]
     │
     ▼
PS — G-Buffer 输出（5个RT）
```

Task Shader 用 Wave 指令做紧凑打包，不需要原子计数器：

```hlsl
if (visible) {
    uint index = WavePrefixCountBits(visible); // 前面有多少可见
    terrainPayload.patchIndices[index] = dtid;
}
uint visibleCount = WaveActiveCountBits(visible);
```

### 5.2 路径二：Compute Shader 剔除 + Indirect Draw

```
CS ([numthreads(128,1,1)])
 ├→ 读 gPatchMeta[idx]
 ├→ AABBInFrustum剔除
 ├→ InterlockedAdd 原子计数
 └→ 可见的写入 IndirectCommand {indexCount=384, instanceCount=1}

Render → DrawIndexedPrimitivesIndirectCount
 └→ instanceCount=0的command被GPU自动跳过
```

### 5.3 路径三：CPU 实例化绘制（最原始）

```cpp
PrepareGPUPathData(frustum); // CPU 视锥体剔除
DrawIndexedInstancePrimitives(384, visibleCount);
```

VS 通过 `SV_InstanceID` 读取 `gPatchMeta[instanceID]`。

### 5.4 路径选择逻辑

```
if (Mesh Shader 可用) → RenderMS()
else if (GPU Cull 可用) → RenderGPUCulled()
else → RenderCPUInstanced()
```

## 六、静态索引池：零每帧 GPU 上传

初始化时把所有可能的情况都预计算好：

```cpp
mIndexPool[步长层级][邻居排列(0~15)] = {起始偏移, 索引数量}
```

- 步长层级：1, 2, 4, 8, ...（最大到 `gridSizeCells / 8`）
- 邻居排列：4 个方向 × 2 种状态 = 16 种

运行时 `GenerateLeafMesh()` 只需 O(1) 查表：

```cpp
const IndexPoolEntry& entry = mIndexPool[strideLevel][perm];
subMeshInfo.firstIndex = entry.start;
subMeshInfo.indexCount = entry.count;
```

**完全没有索引生成和 GPU 上传。**

## 七、三缓冲机制：避免 GPU/CPU 竞态

```cpp
static constexpr uint32_t kFrameInFlightCount = 3;
RCBufferPtr mPatchMetaBuffers[3];
```

每帧按 `mFrameIndex % 3` 写入对应 slot，渲染时也读对应 slot：

```
Update() → 更新四叉树（不写GPU缓冲区）
↓
Render() 入口 → FlushFrameData() → mFrameIndex++ → BuildPatchMetaBuffer()
（在 vkWaitForFences 之后调用，GPU已完成对上一帧的读取）
↓
绑定资源 → mPatchMetaBuffers[mFrameIndex % 3]
↓
GPU 读取 → Dispatch / Draw
```

## 八、PatchMeta 的 SSBO 上传策略

```cpp
如果 已有buffer容量 ≥ 大小 → Map+memcpy原地更新
否则 → 分配更大的buffer（容量 = dataSize × 1.5）
```

惰性扩容避免每帧重新分配 GPU 缓冲区。

## 九、Shader 文件总览

| Shader | 管线 | 用途 |
|---|---|---|
| `TerrainMS.shader` | TS + MS + PS | Mesh Shader G-Buffer（最新）|
| `Terrain.shader` | VS + PS | 传统实例化 G-Buffer |
| `TerrainDepth.shader` | VS + PS | 传统实例化 Depth-only |
| `TerrainCull.shader` | CS | GPU 视锥体剔除 |
| `TerrainCommon.hlsl` | include | PatchMeta + AABBInFrustum 函数 |

`TerrainCommon.hlsl` 中的 `TerrainPayload` 结构体，用于 TS 传给 MS 的 groupshared 数据：

```hlsl
#define AS_GROUP_SIZE 1024
struct TerrainPayload {
    uint patchIndices[AS_GROUP_SIZE];
};
```

## 十、踩坑总结

1. **C++ 和 HLSL 的结构体对齐必须一致** — 加 `static_assert` 来保底。

2. **四叉树邻居查找算法很绕** — `GetMinNeighborLevel` 要做 DFS 下降查找所有相邻叶节点，排除不重叠的部分。

3. **裂缝修复在 Mesh Shader 中的方案跟传统方案完全不同** — 传统路径改索引，MS 路径改顶点高度值。同时维护两条路径有额外成本。

4. **三缓冲是最简洁的 GPU/CPU 竞态解决方案** — 双缓冲在某些情况下 CPU 还会追上 GPU，三缓冲更安全。

5. **Wave Intrinsics 做紧凑打包真的很爽** — 不需要 atomic、不需要 groupsync，但要求 wave size 和工作组大小匹配。

6. **`SetMeshOutputCounts` 必须所有线程调用一致** — 忘了就是 GPU driver crash 或花屏。

7. **`UpdateNode` 阶段做好前向约束检查** — 比事后 `EnforceNeighborConstraint` 循环细分效率更高。