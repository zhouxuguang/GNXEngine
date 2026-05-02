//
//  TerrainComponent.h
//  GNXEngine
//
//  带有专用渲染路径的地形渲染组件。
//  封装了 QuadTreeTerrain、地形材质和每帧 LOD 更新。
//  通过 DeferredSceneRenderer 中的地形专用路径渲染，
//  不走通用的 MeshRenderer + MeshDrawUtil 管线。
//

#ifndef GNXENGINE_TERRAIN_COMPONENT_INCLUDE_H
#define GNXENGINE_TERRAIN_COMPONENT_INCLUDE_H

#include "../RSDefine.h"
#include "../Component.h"
#include "../Material.h"
#include "QuadTreeTerrain.h"
#include "TerrainCullPass.h"
#include "../FrameGraph/FrameGraph.h"           // FrameGraph 集成，用于 GPU 剔除
#include "Runtime/MathUtil/include/Frustum.h"

NS_RENDERSYSTEM_BEGIN

struct RenderInfo;

/**
 * 地形渲染组件。
 *
 * 与基于 MeshRenderer 的地形的区别：
 * - 使用专用的 Terrain.shader / TerrainDepth.shader（非通用 BasePass/DepthGenerate）
 * - 模板网格（17x17）通过 SSBO + 高度图采样在所有 patch 间共享
 * - VS 从 SSBO 读取 PatchMeta[instanceID] 获取每个 patch 的世界变换
 * - VS 采样高度图纹理获取 Y 轴位移
 * - 为 GPU 驱动渲染预留扩展能力（Compute 剔除、Indirect Draw）
 */
class RENDERSYSTEM_API TerrainComponent : public Component
{
public:
    TerrainComponent();
    ~TerrainComponent() override;

    /**
     * 从高度图图像初始化地形。
     */
    void InitFromHeightMap(const char* heightmapPath,
                           float worldSizeXZ = 512.0f,
                           float heightScale = 80.0f,
                           uint32_t maxLevel = 5);

    /**
     * 从程序化噪声初始化地形。
     */
    void InitProcedural(uint32_t gridSize = 513,
                        float worldSizeXZ = 512.0f,
                        float heightScale = 80.0f,
                        uint32_t maxLevel = 5);

    /**
     * 设置地形材质（拥有地形专用 shader + 纹理）。
     */
    void SetMaterial(const MaterialPtr& material);

    /**
     * 每帧更新：根据相机位置进行 LOD 选择。
     */
    void Update(float deltaTime) override;

    /**
     * 通过 GPU 驱动渲染路径渲染地形（G-Buffer 通道）。
     *
     * 使用模板网格 + PatchMeta SSBO + 高度图纹理。
     * VS 从 PatchMeta[instanceID] 读取每个 patch 的世界变换。
     *
     * @param renderEncoder      当前渲染编码器（在 G-Buffer 渲染 pass 内）
     * @param cameraUBO          相机 Uniform 缓冲区
     * @param objectUBO          对象（模型矩阵）Uniform 缓冲区
     * @param terrainGBufferPSO  地形专用 G-Buffer 图形管线
     * @param frustum            相机视锥体，用于逐叶节点剔除（nullptr = 全部绘制）
     */
    void Render(class RenderEncoder* renderEncoder,
                UniformBufferPtr cameraUBO,
                UniformBufferPtr objectUBO,
                GraphicsPipelinePtr terrainGBufferPSO,
                const mathutil::Frustumf* frustum = nullptr);

    /**
     * 仅渲染地形深度（用于 PreDepth pass）。
     *
     * @param renderEncoder     当前渲染编码器（在深度渲染 pass 内）
     * @param cameraUBO         相机 Uniform 缓冲区
     * @param objectUBO         对象（模型矩阵）Uniform 缓冲区
     * @param terrainDepthPSO   地形专用仅深度图形管线
     * @param frustum           相机视锥体，用于逐叶节点剔除（nullptr = 全部绘制）
     */
    void RenderDepthOnly(class RenderEncoder* renderEncoder,
                         UniformBufferPtr cameraUBO,
                         UniformBufferPtr objectUBO,
                         GraphicsPipelinePtr terrainDepthPSO,
                         const mathutil::Frustumf* frustum = nullptr);

    // ---- 访问器 ----

    QuadTreeTerrainPtr GetQuadTreeTerrain() const { return mQuadTreeTerrain; }

    float GetHeight(float worldX, float worldZ) const;

    void SetLODDistanceFactor(float factor);
    void SetSSEThreshold(float threshold);

    bool IsInitialized() const { return mQuadTreeTerrain != nullptr; }

    /**
     * 切换线框模式，用于观察 LOD 变化。
     */
    void SetWireframe(bool wireframe);

    /**
     * 启用/禁用 GPU Compute Shader 剔除（默认：开启）。
     * 启用时使用 TerrainCullPass（CS 视锥体剔除 → Indirect Draw）。
     * 禁用时使用 CPU 视锥体剔除 + 实例化绘制（原始路径）。
     */
    void SetUseGPUCulling(bool enable);
    bool IsUsingGPUCulling() const { return mUseGPUCulling; }

    /**
     * 执行 GPU 剔除 Compute Dispatch（GPU 剔除启用时必须在 Render() 之前调用）。
     *
     * 对所有 patch 运行 TerrainCull CS，输出 IndirectCommand 缓冲区。
     * 结果被缓存，由 Render()/RenderDepthOnly() 消费。
     *
     * @param commandBuffer  用于创建 ComputeEncoder 的命令缓冲区
     * @param vpMatrix       视图-投影矩阵（供 shader 提取视锥体平面）
     * @param cameraUBO      相机 Uniform 缓冲区（cbPerCamera，向 shader 提供 MATRIX_VP）
     */
    void DispatchGPUCull(CommandBufferPtr commandBuffer,
                         const mathutil::Matrix4x4f& vpMatrix,
                         RenderCore::UniformBufferPtr cameraUBO = nullptr);

    /**
     * 以 FrameGraph Compute Pass 形式执行 GPU 剔除（推荐，优于 DispatchGPUCull）。
     *
     * 将 TerrainCull CS 作为正规的 compute pass 注册到 FrameGraph 中。
     * 必须在帧设置期间调用（在 PreDepthPass + BasePass 之前），
     * 这样 Render()/RenderDepthOnly() 执行时间接参数缓冲区已就绪。
     *
     * @param frameGraph     要注册剔除 pass 的 FrameGraph
     * @param commandBuffer  用于创建 ComputeEncoder 的命令缓冲区
     * @param vpMatrix       视图-投影矩阵（供 shader 提取视锥体平面）
     * @param cameraUBO      相机 Uniform 缓冲区（cbPerCamera，向 shader 提供 MATRIX_VP）
     */
    void DispatchCullViaFrameGraph(FrameGraph& frameGraph,
                                    CommandBufferPtr commandBuffer,
                                    const mathutil::Matrix4x4f& vpMatrix,
                                    RenderCore::UniformBufferPtr cameraUBO = nullptr);

private:
    /**
     * 确保地形专用 UBO（cbTerrain）已创建且数据最新。
     */
    void EnsureTerrainUBO();

    /**
     * 构建 GPU 路径数据（视锥体剔除 + 可见 PatchMeta SSBO）。
     * 每帧在首次渲染调用之前调用一次。
     */
    void PrepareGPUPathData(const mathutil::Frustumf* frustum);

    /**
     * GPU 剔除间接绘制路径（G-Buffer）。
     * 使用 DispatchGPUCull() 的 mLastCullOutput.indirectArgsBuffer。
     */
    void RenderGPUCulled(class RenderEncoder* renderEncoder,
                         UniformBufferPtr cameraUBO,
                         GraphicsPipelinePtr terrainGBufferPSO);

    /**
     * CPU 实例化绘制路径（G-Buffer，原始路径）。
     * 使用 CPU 视锥体剔除 + DrawIndexedInstancePrimitives。
     */
    void RenderCPUInstanced(class RenderEncoder* renderEncoder,
                            UniformBufferPtr cameraUBO,
                            GraphicsPipelinePtr terrainGBufferPSO,
                            const mathutil::Frustumf* frustum);

    /**
     * GPU 剔除间接绘制路径（仅深度）。
     */
    void RenderDepthGPUCulled(class RenderEncoder* renderEncoder,
                              UniformBufferPtr cameraUBO,
                              GraphicsPipelinePtr terrainDepthPSO);

    /**
     * CPU 实例化绘制路径（仅深度，原始路径）。
     */
    void RenderDepthCPUInstanced(class RenderEncoder* renderEncoder,
                                 UniformBufferPtr cameraUBO,
                                 GraphicsPipelinePtr terrainDepthPSO,
                                 const mathutil::Frustumf* frustum);

    /**
     * 绑定材质漫反射纹理（G-Buffer 和 Depth 路径共用）。
     */
    void BindMaterialTextures(class RenderEncoder* renderEncoder);

    QuadTreeTerrainPtr mQuadTreeTerrain;
    MaterialPtr      mMaterial;
    bool             mWireframe = false;

    // 地形参数 UBO（shader 中的 cbTerrain）
    UniformBufferPtr mTerrainParamsUBO;
    bool mTerrainParamsDirty = true;

    // GPU 路径数据准备标志（每帧重置）
    bool mGPUPathDataPrepared = false;

    // GPU Compute Shader 剔除 pass（GPU 视锥体剔除 → 间接绘制）
    TerrainCullPassPtr mCullPass = nullptr;
    bool mUseGPUCulling = false;   // 默认：使用 GPU 剔除（Compute Shader → Indirect Draw）

    // 上次 DispatchGPUCull() 调用的缓存输出（由 Render/RenderDepthOnly 消费）
    TerrainCullOutput mLastCullOutput;

    // cbTerrain 结构体，匹配 shader 布局
    struct cbTerrainParams
    {
        float worldSize;
        float halfWorldSize;
        float uvTileScale;
        uint32_t gridSize;
    };
};

typedef std::shared_ptr<TerrainComponent> TerrainComponentPtr;

NS_RENDERSYSTEM_END

#endif // GNXENGINE_TERRAIN_COMPONENT_INCLUDE_H
