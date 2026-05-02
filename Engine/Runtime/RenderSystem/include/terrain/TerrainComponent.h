//
//  TerrainComponent.h
//  GNXEngine
//
//  Terrain rendering component with dedicated rendering path.
//  Encapsulates QuadTreeTerrain, terrain material, and per-frame LOD updates.
//  Rendered through the terrain-specific path in DeferredSceneRenderer,
//  NOT through the generic MeshRenderer + MeshDrawUtil pipeline.
//

#ifndef GNXENGINE_TERRAIN_COMPONENT_INCLUDE_H
#define GNXENGINE_TERRAIN_COMPONENT_INCLUDE_H

#include "../RSDefine.h"
#include "../Component.h"
#include "../Material.h"
#include "QuadTreeTerrain.h"
#include "TerrainCullPass.h"
#include "../FrameGraph/FrameGraph.h"           // FrameGraph integration for GPU culling
#include "Runtime/MathUtil/include/Frustum.h"

NS_RENDERSYSTEM_BEGIN

struct RenderInfo;

/**
 * Terrain rendering component.
 *
 * Key differences from MeshRenderer-based terrain:
 * - Uses dedicated Terrain.shader/TerrainDepth.shader (not generic BasePass/DepthGenerate)
 * - Template mesh (17x17) shared across all patches via SSBO + heightmap sampling
 * - VS reads PatchMeta[instanceID] from SSBO for per-patch world transform
 * - VS samples heightmap Texture for Y displacement
 * - Future-proof for GPU-driven rendering (Compute culling, Indirect Draw)
 */
class RENDERSYSTEM_API TerrainComponent : public Component
{
public:
    TerrainComponent();
    ~TerrainComponent() override;

    /**
     * Initialize terrain from a heightmap image.
     */
    void InitFromHeightMap(const char* heightmapPath,
                           float worldSizeXZ = 512.0f,
                           float heightScale = 80.0f,
                           uint32_t maxLevel = 5);

    /**
     * Initialize terrain from procedural noise.
     */
    void InitProcedural(uint32_t gridSize = 513,
                        float worldSizeXZ = 512.0f,
                        float heightScale = 80.0f,
                        uint32_t maxLevel = 5);

    /**
     * Set the terrain material (owns the terrain-specific shader + textures).
     */
    void SetMaterial(const MaterialPtr& material);

    /**
     * Per-frame update: LOD selection based on camera position.
     */
    void Update(float deltaTime) override;

    /**
     * Render the terrain through the GPU-driven rendering path (G-Buffer pass).
     *
     * Uses template mesh + PatchMeta SSBO + heightmap texture.
     * VS reads PatchMeta[instanceID] for per-patch world transform.
     *
     * @param renderEncoder      Active render encoder (within G-Buffer render pass)
     * @param cameraUBO          Camera uniform buffer
     * @param objectUBO          Object (model matrix) uniform buffer
     * @param terrainGBufferPSO  Terrain-specific G-Buffer graphics pipeline
     * @param frustum            Camera frustum for per-leaf culling (nullptr = draw all)
     */
    void Render(class RenderEncoder* renderEncoder,
                UniformBufferPtr cameraUBO,
                UniformBufferPtr objectUBO,
                GraphicsPipelinePtr terrainGBufferPSO,
                const mathutil::Frustumf* frustum = nullptr);

    /**
     * Render terrain depth only (for PreDepth pass).
     *
     * @param renderEncoder     Active render encoder (within depth render pass)
     * @param cameraUBO         Camera uniform buffer
     * @param objectUBO         Object (model matrix) uniform buffer
     * @param terrainDepthPSO   Terrain-specific depth-only graphics pipeline
     * @param frustum           Camera frustum for per-leaf culling (nullptr = draw all)
     */
    void RenderDepthOnly(class RenderEncoder* renderEncoder,
                         UniformBufferPtr cameraUBO,
                         UniformBufferPtr objectUBO,
                         GraphicsPipelinePtr terrainDepthPSO,
                         const mathutil::Frustumf* frustum = nullptr);

    // ---- Accessors ----

    QuadTreeTerrainPtr GetQuadTreeTerrain() const { return mQuadTreeTerrain; }

    float GetHeight(float worldX, float worldZ) const;

    void SetLODDistanceFactor(float factor);
    void SetSSEThreshold(float threshold);

    bool IsInitialized() const { return mQuadTreeTerrain != nullptr; }

    /**
     * Toggle wireframe mode for observing LOD changes.
     */
    void SetWireframe(bool wireframe);

    /**
     * Enable/disable GPU Compute Shader culling (default: off).
     * When enabled, uses TerrainCullPass (CS frustum cull → Indirect Draw).
     * When disabled, uses CPU frustum culling + instanced draw (original path).
     */
    void SetUseGPUCulling(bool enable);
    bool IsUsingGPUCulling() const { return mUseGPUCulling; }

    /**
     * Execute GPU culling compute dispatch (must call BEFORE Render() when GPU culling is enabled).
     *
     * Runs TerrainCull CS on all patches, outputs IndirectCommand buffer.
     * The result is cached and consumed by Render()/RenderDepthOnly().
     *
     * @param commandBuffer  Command buffer for creating ComputeEncoder
     * @param vpMatrix       View-Projection matrix (for frustum plane extraction in shader)
     * @param cameraUBO      Camera uniform buffer (cbPerCamera, provides MATRIX_VP to shader)
     */
    void DispatchGPUCull(CommandBufferPtr commandBuffer,
                         const mathutil::Matrix4x4f& vpMatrix,
                         RenderCore::UniformBufferPtr cameraUBO = nullptr);

    /**
     * Execute GPU culling as a FrameGraph Compute Pass (preferred over DispatchGPUCull).
     *
     * Registers TerrainCull CS into FrameGraph as a proper compute pass.
     * Must be called during frame setup (before PreDepthPass + BasePass),
     * so the indirect args buffer is ready when Render()/RenderDepthOnly() execute.
     *
     * @param frameGraph     FrameGraph to register the cull pass into
     * @param commandBuffer  Command buffer for creating ComputeEncoder
     * @param vpMatrix       View-Projection matrix (for frustum plane extraction in shader)
     * @param cameraUBO      Camera uniform buffer (cbPerCamera, provides MATRIX_VP to shader)
     */
    void DispatchCullViaFrameGraph(FrameGraph& frameGraph,
                                    CommandBufferPtr commandBuffer,
                                    const mathutil::Matrix4x4f& vpMatrix,
                                    RenderCore::UniformBufferPtr cameraUBO = nullptr);

private:
    /**
     * Ensure terrain-specific UBO (cbTerrain) is created and up-to-date.
     */
    void EnsureTerrainUBO();

    /**
     * Build GPU path data (frustum culling + visible PatchMeta SSBO).
     * Called once per frame before the first render call.
     */
    void PrepareGPUPathData(const mathutil::Frustumf* frustum);

    /**
     * GPU-culled indirect draw path (G-Buffer).
     * Uses mLastCullOutput.indirectArgsBuffer from DispatchGPUCull().
     */
    void RenderGPUCulled(class RenderEncoder* renderEncoder,
                         UniformBufferPtr cameraUBO,
                         GraphicsPipelinePtr terrainGBufferPSO);

    /**
     * CPU instanced draw path (G-Buffer, original).
     * Uses CPU frustum culling + DrawIndexedInstancePrimitives.
     */
    void RenderCPUInstanced(class RenderEncoder* renderEncoder,
                            UniformBufferPtr cameraUBO,
                            GraphicsPipelinePtr terrainGBufferPSO,
                            const mathutil::Frustumf* frustum);

    /**
     * GPU-culled indirect draw path (Depth-only).
     */
    void RenderDepthGPUCulled(class RenderEncoder* renderEncoder,
                              UniformBufferPtr cameraUBO,
                              GraphicsPipelinePtr terrainDepthPSO);

    /**
     * CPU instanced draw path (Depth-only, original).
     */
    void RenderDepthCPUInstanced(class RenderEncoder* renderEncoder,
                                 UniformBufferPtr cameraUBO,
                                 GraphicsPipelinePtr terrainDepthPSO,
                                 const mathutil::Frustumf* frustum);

    /**
     * Bind material diffuse texture (shared by G-Buffer and Depth paths).
     */
    void BindMaterialTextures(class RenderEncoder* renderEncoder);

    QuadTreeTerrainPtr mQuadTreeTerrain;
    MaterialPtr      mMaterial;
    bool             mWireframe = false;

    // Terrain params UBO (cbTerrain in shader)
    UniformBufferPtr mTerrainParamsUBO;
    bool mTerrainParamsDirty = true;

    // GPU path data prepared flag (reset each frame)
    bool mGPUPathDataPrepared = false;

    // GPU Compute Shader culling pass (frustum cull on GPU → indirect draw)
    TerrainCullPassPtr mCullPass = nullptr;
    bool mUseGPUCulling = false;   // default: use GPU culling (Compute Shader → Indirect Draw)

    // Cached output from last DispatchGPUCull() call (consumed by Render/RenderDepthOnly)
    TerrainCullOutput mLastCullOutput;

    // cbTerrain struct matching shader layout
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
