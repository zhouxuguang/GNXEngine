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

    bool IsWireframe() const { return mWireframe; }

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

    QuadTreeTerrainPtr mQuadTreeTerrain;
    MaterialPtr      mMaterial;
    bool             mWireframe = false;

    // Terrain params UBO (cbTerrain in shader)
    UniformBufferPtr mTerrainParamsUBO;
    bool mTerrainParamsDirty = true;

    // GPU path data prepared flag (reset each frame)
    bool mGPUPathDataPrepared = false;

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
