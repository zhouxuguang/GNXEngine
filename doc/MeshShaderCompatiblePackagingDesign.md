# GNXEngine Mesh Shader 与传统 Vertex Pipeline 兼容打包设计

## 结论

为了同时支持 Mesh Shader 和传统 Vertex Pipeline，最稳妥的方案是：

> 同一个 `.meshasset` 保存一份共享顶点数据，同时包含传统 Index/LOD 数据和可选 Meshlet 数据。运行时根据 GPU 能力选择渲染路径。

不要生成两套完全独立、互不关联的 Mesh，因为容易导致材质边界、LOD、蒙皮和顶点属性不一致。

碰撞数据应继续作为独立可选 Chunk。高端平台可以打包复杂碰撞 Mesh，移动端可以换成简化 Mesh 或 Convex Hull，但它与 Mesh Shader/Vertex Pipeline 的选择无关。

## 双路径结构

```text
                         ┌─ Traditional Pipeline
共享 Vertex Buffer ──────┤   Index Buffer → DrawIndexed
                         │
                         └─ Mesh Shader Pipeline
                             MeshletDesc
                             MeshletVertexIndices
                             MeshletTriangleIndices
                             → DispatchMesh
```

关键原则：

- 两条管线共享同一套顶点编号和 Vertex Buffer。
- Meshlet 顶点表保存“局部顶点 → 全局顶点”的映射。
- 传统管线使用全局 Index Buffer。
- Mesh Shader 使用 Meshlet 内部局部三角形索引。
- SubMesh、MaterialSlot、LOD、Bounds 在两条管线之间保持一致。
- 运行时只选择提交方式，不改变 Mesh 的逻辑身份。

## 当前 GNXEngine 的差距

目前普通 Mesh 容器只有：

- `Engine/Runtime/AssetManager/source/MeshMessage.proto:24`

```text
vertexData
indiceData
indiceType
vertexCount
indiceCount
vertexSize
subMeshInfos
vertexChannelInfos
```

它没有：

- LOD 数据
- Meshlet 描述
- Meshlet 顶点索引
- Meshlet 局部三角形
- Meshlet Bounds/Cone
- SubMesh 对应的 Meshlet 范围
- Mesh Shader 数据版本
- 可选 Chunk 信息

引擎已经有独立的 `meshlet_gen`，生成的数据包括：

```text
vertexPositions
lodMeshletOffsets
lodMeshletCounts
meshlets
meshletVertices
meshletTriangles
meshletPartitions
```

但它目前使用另一套裸二进制格式，没有进入 `.meshasset` 容器。

更重要的是，当前工具按“位置相同”重新去重顶点：

- `tool/meshlet_gen/main.cpp:60`

这对于正式渲染有风险。两个顶点即使位置相同，也可能因为以下属性不同而必须保持分离：

- 法线
- 切线
- UV
- 顶点色
- 骨骼索引
- 骨骼权重
- Material/SubMesh 边界

因此，正式 Meshlet 构建不能只对 Position 去重。Meshlet 中的全局顶点索引必须指向最终 Cook 后的完整渲染顶点。

## 推荐的 `.meshasset` 容器

建议从单一 protobuf 大字段逐步升级为“元数据 + 可选二进制 Chunk”结构：

```text
MeshAsset
├─ Header
│  ├─ version
│  ├─ flags
│  ├─ profile
│  ├─ bounds
│  ├─ vertexLayoutHash
│  └─ meshletLayoutVersion
│
├─ SharedVertexData
│  ├─ VertexStreams[]
│  └─ VertexLayout
│
├─ LODs[]
│  ├─ bounds
│  ├─ screenSize/error
│  │
│  ├─ TraditionalData
│  │  ├─ indexBuffer
│  │  └─ subMeshes[]
│  │
│  └─ MeshShaderData（可选）
│     ├─ meshlets[]
│     ├─ meshletVertexIndices[]
│     ├─ meshletTriangles[]
│     ├─ meshletBounds[]
│     └─ subMeshMeshletRanges[]
│
├─ Skinning（可选）
├─ ClusterHierarchy（可选）
└─ Collision（可选）
```

至少需要以下标志：

```cpp
enum MeshAssetFlags : uint32_t
{
    HasTraditionalIndices = 1 << 0,
    HasMeshlets           = 1 << 1,
    HasClusterHierarchy   = 1 << 2,
    HasSkinning           = 1 << 3,
    HasCollision          = 1 << 4,
};
```

## 共享顶点数据设计

### Meshlet 引用全局顶点

推荐结构：

```cpp
struct MeshletDesc
{
    uint32_t vertexOffset;
    uint32_t vertexCount;

    uint32_t triangleOffset;
    uint32_t triangleCount;

    uint32_t materialSlot;
    uint32_t lodIndex;

    BoundingSphere bounds;
    PackedNormal coneAxis;
    float coneCutoff;
};
```

全局顶点映射：

```cpp
std::vector<uint32_t> meshletVertexIndices;
```

局部三角形：

```cpp
struct MeshletTriangle
{
    uint8_t i0;
    uint8_t i1;
    uint8_t i2;
};
```

例如某个 Meshlet：

```text
meshletVertexIndices = [102, 501, 88, 300]
meshletTriangles     = [(0,1,2), (2,1,3)]
```

Mesh Shader 中：

```hlsl
uint globalIndex = MeshletVertexIndices[
    meshlet.vertexOffset + localVertexIndex];

Vertex vertex = Vertices[globalIndex];
```

这样 Mesh Shader 与传统管线读取的是同一份 Vertex Buffer。

### 不建议只保存 Meshlet 重排后的顶点

如果 Meshlet 路径使用一套复制、重排过的顶点，而传统路径使用另一套顶点：

- GPU 内存增加。
- 蒙皮需要更新两份数据。
- Morph Target 需要维护两份映射。
- Ray Tracing BLAS 和传统 Index Buffer 很难共用。
- 顶点修改和流式加载更复杂。

初期应优先共享全局顶点数据。

## LOD 同时生成两种表示

推荐先生成每级 LOD 的标准三角网格，再从该 LOD 构建 Meshlet：

```text
源 Mesh
   ↓
生成 LOD0 三角网格
   ├─ LOD0 Index Buffer
   └─ LOD0 Meshlets

   ↓ Simplify
生成 LOD1 三角网格
   ├─ LOD1 Index Buffer
   └─ LOD1 Meshlets

   ↓ Simplify
生成 LOD2 三角网格
   ├─ LOD2 Index Buffer
   └─ LOD2 Meshlets
```

这样可以保证：

- 两条渲染路径的几何结果相同。
- LOD 切换规则相同。
- 包围盒和几何误差一致。
- Material/SubMesh 划分一致。

不建议由 Meshlet Hierarchy 反向生成传统 Index Buffer。传统 LOD 和 Meshlet LOD 都应来自同一个离线中间表示。

## SubMesh 与材质边界

Meshlet 不应跨越 Material Slot。

例如：

```text
SubMesh 0 → BodyMaterial
SubMesh 1 → GlassMaterial
SubMesh 2 → HairMaterial
```

Cook 时应分别构建：

```text
LOD0
├─ SubMesh 0
│  ├─ traditional index range
│  └─ meshlet range
├─ SubMesh 1
│  ├─ traditional index range
│  └─ meshlet range
└─ SubMesh 2
   ├─ traditional index range
   └─ meshlet range
```

元数据可定义为：

```cpp
struct SubMeshLOD
{
    uint32_t materialSlot;

    uint32_t firstIndex;
    uint32_t indexCount;

    uint32_t firstMeshlet;
    uint32_t meshletCount;
};
```

传统管线使用 `firstIndex/indexCount`，Mesh Shader 使用 `firstMeshlet/meshletCount`。

## 蒙皮 Mesh 兼容

蒙皮 Mesh 也应共享顶点数据：

```text
Static Vertex Attributes
├─ position
├─ normal
├─ tangent
└─ uv

Skinning Attributes
├─ boneIndices
└─ boneWeights
```

### 方案 A：先 Compute Skinning

```text
Bind Pose Vertex Buffer
        ↓ Compute Skinning
Skinned Vertex Buffer
        ├─ Traditional Vertex Pipeline
        └─ Mesh Shader Pipeline
```

这是更适合当前引擎的方案，因为两条路径可以共享蒙皮后的 Vertex Buffer。

### 方案 B：Mesh Shader 内蒙皮

Mesh Shader 自己读取骨骼矩阵并计算顶点位置，但传统管线仍需在 VS 中蒙皮。这种方式会出现两套 Shader 实现，初期不建议优先采用。

## 运行时选择逻辑

```cpp
enum class MeshRenderPath
{
    Traditional,
    MeshShader
};

MeshRenderPath SelectMeshRenderPath(
    const RenderDeviceCapabilities& caps,
    const MeshAsset& asset,
    const RenderSettings& settings)
{
    if (settings.enableMeshShader &&
        caps.supportsMeshShader &&
        asset.HasMeshlets())
    {
        return MeshRenderPath::MeshShader;
    }

    return MeshRenderPath::Traditional;
}
```

传统路径：

```text
CreateVertexBuffer
CreateIndexBuffer
DrawIndexed
```

Mesh Shader 路径：

```text
CreateVertexBuffer
CreateMeshletBuffer
CreateMeshletVertexIndexBuffer
CreateMeshletTriangleBuffer
DispatchMesh
```

不能只检查 GPU 支持，还要同时检查当前资产是否包含 Meshlet Chunk。

## 平台打包策略

```yaml
mesh:
  traditionalPipeline: true
  meshShader: true
  clusterHierarchy: false

platformOverrides:
  WindowsHigh:
    traditionalPipeline: true
    meshShader: true
    clusterHierarchy: true

  AndroidHigh:
    traditionalPipeline: true
    meshShader: true
    clusterHierarchy: false

  AndroidLow:
    traditionalPipeline: true
    meshShader: false
    clusterHierarchy: false
```

推荐策略：

| Profile | Traditional Index | Meshlet | Cluster Hierarchy |
|---|---:|---:|---:|
| Desktop High | 保留 | 保留 | 可选 |
| Desktop Compatible | 保留 | 可选 | 不保留 |
| Mobile High | 保留 | 视设备能力 | 不一定保留 |
| Mobile Low | 保留 | 不保留 | 不保留 |
| Mesh-Shader-only 特殊包 | 可裁剪 | 保留 | 可选 |

在兼容阶段，建议所有客户端都保留传统 Index Buffer，因为它：

- 提供不支持 Mesh Shader 设备的 fallback。
- 可用于深度、阴影或特殊 Pass。
- 可用于调试和编辑器显示。
- 可用于 Ray Tracing BLAS 构建。
- 相对于顶点数据，额外 Index Buffer 通常不算特别昂贵。

只有明确要求 Mesh-Shader-only 的平台包，才考虑删除传统 Index Buffer。

## 对现有 Meshlet 代码的建议

当前常量：

- `Engine/Runtime/RenderSystem/include/meshlet/MeshLetCommon.h:23`

```cpp
kMeshletMaxVertices  = 64;
kMeshletMaxTriangles = 124;
```

这组限制可作为初始跨平台值，适合局部索引使用 `uint8_t`。但必须把它记录在 Cook 配置和格式版本中：

```cpp
struct MeshletCookSettings
{
    uint32_t maxVertices = 64;
    uint32_t maxTriangles = 124;
    bool buildCone = true;
    bool buildHierarchy = false;
    uint32_t formatVersion = 1;
};
```

当前 `meshlet_gen` 的裸二进制序列化存在以下发布风险：

- 直接写 C++ `Meshlet` 结构体，可能受 padding 和 ABI 影响。
- 没有魔数和文件版本。
- 没有 endian 标记。
- 没有 Chunk offset/size。
- 缺少完整 Vertex Layout。
- 使用只包含 Position 的独立顶点集合。
- 没有正式进入 `.meshasset` 和 AssetManifest。

因此它目前更适合作为算法验证工具，而不是最终资产打包工具。

## protobuf/Chunk 改造建议

不建议继续把所有大块 GPU 数据直接塞进一个 protobuf。可以让 protobuf 只保存结构化元数据，大块数组放 Chunk：

```proto
message MeshAssetMessage
{
    uint32 version = 1;
    uint32 flags = 2;
    repeated VertexStreamMessage vertexStreams = 3;
    repeated MeshLODMessage lods = 4;
    BoundsMessage bounds = 5;
}

message MeshLODMessage
{
    float geometricError = 1;
    repeated SubMeshLODMessage subMeshes = 2;

    ChunkRef indexData = 3;
    ChunkRef meshletDescs = 4;
    ChunkRef meshletVertices = 5;
    ChunkRef meshletTriangles = 6;
}
```

Chunk 表：

```cpp
struct MeshChunkEntry
{
    MeshChunkType type;
    uint32_t lod;
    uint64_t offset;
    uint64_t compressedSize;
    uint64_t uncompressedSize;
    uint64_t contentHash;
};
```

优点：

- 可以只加载 LOD0 或某一级 LOD。
- 不支持 Mesh Shader 时不读取 Meshlet Chunk。
- 可以为各 Chunk 独立压缩。
- 后续容易接入资源流送。
- 不需要 protobuf 为巨大字节数组申请整块临时内存。

## 推荐 Cook 流程

```text
1. Assimp 导入源模型
2. 按完整顶点属性建立 canonical vertices
3. 保留 SubMesh/Material 边界
4. 优化顶点缓存和 Overdraw
5. 生成各级 LOD 三角网格
6. 为每级 LOD 生成传统 Index Buffer
7. 按 SubMesh 为每级 LOD 构建 Meshlet
8. 计算 Meshlet Bounds、Normal Cone 和 LOD Error
9. 可选生成 Cluster Hierarchy
10. 序列化共享 Vertex Streams
11. 序列化 Traditional 和 Meshlet Chunks
12. 写入平台 DerivedData
13. 生成 Manifest
14. 拷贝到目标平台 Staging
```

第 2 步至关重要：必须先确定最终渲染顶点，之后传统 Index 和 Meshlet 全局索引都引用它。

## 分阶段实现建议

### 第一阶段

1. 扩展 `.meshasset`，支持版本和可选 Meshlet 数据。
2. 保留现有 Vertex Buffer 和 UInt32 Index Buffer。
3. 从现有完整 Mesh 索引直接构建 Meshlet。
4. 删除正式链路中的 Position-only 去重。
5. 为每个 SubMesh 分别生成 Meshlet。
6. 运行时按能力选择 `DrawIndexed` 或 `DispatchMesh`。

### 第二阶段

1. 增加多 LOD。
2. 增加 Meshlet Bounds 和 Cone Culling。
3. 增加 GPU-driven indirect dispatch。
4. 支持 UInt16 全局索引。
5. 增加蒙皮 Mesh。
6. 增加分 Chunk 加载。

### 第三阶段

1. Cluster Hierarchy。
2. 几何流送。
3. 页面化顶点和 Meshlet 数据。
4. Nanite 类 GPU 细粒度 LOD。
5. 按平台裁剪传统 Index 数据。

另外，当前 `MeshMessageUtil::DecodeMeshMessage()` 无论 `indiceType` 是什么，都把索引当作 `uint32_t` 解码：

- `Engine/Runtime/AssetManager/source/MeshMessageUtil.cpp:145`

因此，在引入 UInt16 index 之前需要先修正这里。

## 最终方案

```text
一个 Mesh 逻辑资产
    ├─ 共享 Vertex Streams
    ├─ Traditional Index Chunks
    ├─ Meshlet Chunks（可选）
    ├─ Cluster Hierarchy（可选）
    └─ Collision Chunks（独立可选）

运行时：
    GPU 支持 Mesh Shader + 资产有 Meshlet → Mesh Shader
    否则                              → DrawIndexed
```

这样既能利用 Mesh Shader，又能保持传统 Vertex Pipeline、移动端低端设备、编辑器和特殊渲染 Pass 的兼容性。

