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
 * - Uses dedicated Terrain.shader (not generic GBufferPBR)
 * - Single PSO/material bind for all patches (vs N binds for N submeshes)
 * - Per-leaf frustum culling before draw calls
 * - Future-proof for GPU-driven rendering (Indirect Draw, Compute culling)
 */
class RENDERSYSTEM_API TerrainComponent : public Component
{
public:
    TerrainComponent();
    ~TerrainComponent() override;

    /**
     * Initialize terrain from a heightmap image.
     * @param heightmapPath  Path to GRAY8/GRAY16 heightmap
     * @param worldSizeXZ    World-space extent in X and Z
     * @param heightScale    Multiplier for height values
     * @param maxLevel       Maximum quadtree subdivision level
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
     * Called by the scene system or the demo's RenderFrame().
     */
    void Update(float deltaTime) override;

    /**
     * Render the terrain through a dedicated rendering path.
     * Called by DeferredSceneRenderer during the G-Buffer pass.
     *
     * Sets PSO + material once, then draws all visible leaf nodes.
     * This avoids the N x PSO-bind overhead of MeshDrawUtil.
     *
     * @param renderEncoder  Active render encoder (within G-Buffer render pass)
     * @param cameraUBO      Camera uniform buffer
     * @param objectUBO      Object (model matrix) uniform buffer
     * @param basePassPSO    The G-Buffer graphics pipeline state object
     * @param frustum        Camera frustum for per-leaf culling (nullptr = draw all)
     */
    void Render(class RenderEncoder* renderEncoder,
                UniformBufferPtr cameraUBO,
                UniformBufferPtr objectUBO,
                GraphicsPipelinePtr basePassPSO,
                const mathutil::Frustumf* frustum = nullptr);

    /**
     * Render terrain depth only (for PreDepth pass).
     * Only binds position vertex buffer + depth PSO, then draws all leaf nodes.
     *
     * @param renderEncoder  Active render encoder (within depth render pass)
     * @param cameraUBO      Camera uniform buffer
     * @param objectUBO      Object (model matrix) uniform buffer
     * @param depthPSO       Depth-only graphics pipeline state object
     * @param frustum        Camera frustum for per-leaf culling (nullptr = draw all)
     */
    void RenderDepthOnly(class RenderEncoder* renderEncoder,
                         UniformBufferPtr cameraUBO,
                         UniformBufferPtr objectUBO,
                         GraphicsPipelinePtr depthPSO,
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
    QuadTreeTerrainPtr mQuadTreeTerrain;
    MaterialPtr      mMaterial;
    bool             mWireframe = false;

    // Indirect draw buffer (created lazily, updated per frame)
    RenderCore::RCBufferPtr mIndirectBuffer;
    uint32_t              mIndirectBufferSize = 0;
};

typedef std::shared_ptr<TerrainComponent> TerrainComponentPtr;

NS_RENDERSYSTEM_END

#endif // GNXENGINE_TERRAIN_COMPONENT_INCLUDE_H
