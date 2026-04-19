//
//  TerrainComponent.h
//  GNXEngine
//
//  Terrain rendering component with dedicated rendering path.
//  Encapsulates GeoMipTerrain, terrain material, and per-frame LOD updates.
//  Rendered through the terrain-specific path in DeferredSceneRenderer,
//  NOT through the generic MeshRenderer + MeshDrawUtil pipeline.
//

#ifndef GNXENGINE_TERRAIN_COMPONENT_INCLUDE_H
#define GNXENGINE_TERRAIN_COMPONENT_INCLUDE_H

#include "../RSDefine.h"
#include "../Component.h"
#include "../Material.h"
#include "GeoMipTerrain.h"

NS_RENDERSYSTEM_BEGIN

struct RenderInfo;

/**
 * Terrain rendering component.
 *
 * Key differences from MeshRenderer-based terrain:
 * - Uses dedicated Terrain.shader (not generic GBufferPBR)
 * - Single PSO/material bind for all patches (vs N binds for N submeshes)
 * - Per-patch frustum culling before draw calls
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
     * @param patchSize      Vertices per patch edge (must be 2^n + 1, e.g. 33)
     */
    void InitFromHeightMap(const char* heightmapPath,
                           float worldSizeXZ = 512.0f,
                           float heightScale = 80.0f,
                           uint32_t patchSize = 33);

    /**
     * Initialize terrain from procedural noise.
     */
    void InitProcedural(uint32_t gridSize = 513,
                        float worldSizeXZ = 512.0f,
                        float heightScale = 80.0f,
                        uint32_t patchSize = 33);

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
     * Sets PSO + material once, then draws all visible patches.
     * This avoids the N x PSO-bind overhead of MeshDrawUtil.
     *
     * @param renderEncoder  Active render encoder (within G-Buffer render pass)
     * @param cameraUBO      Camera uniform buffer
     * @param objectUBO      Object (model matrix) uniform buffer
     * @param basePassPSO    The G-Buffer graphics pipeline state object
     */
    void Render(class RenderEncoder* renderEncoder,
                UniformBufferPtr cameraUBO,
                UniformBufferPtr objectUBO,
                GraphicsPipelinePtr basePassPSO);

    /**
     * Render terrain depth only (for PreDepth pass).
     * Only binds position vertex buffer + depth PSO, then draws all patches.
     *
     * @param renderEncoder  Active render encoder (within depth render pass)
     * @param cameraUBO      Camera uniform buffer
     * @param objectUBO      Object (model matrix) uniform buffer
     * @param depthPSO       Depth-only graphics pipeline state object
     */
    void RenderDepthOnly(class RenderEncoder* renderEncoder,
                         UniformBufferPtr cameraUBO,
                         UniformBufferPtr objectUBO,
                         GraphicsPipelinePtr depthPSO);

    // ---- Accessors ----

    GeoMipTerrainPtr GetGeoMipTerrain() const { return mGeoMipTerrain; }

    float GetHeight(float worldX, float worldZ) const;

    void SetLODDistances(const std::vector<float>& distances);

    bool IsInitialized() const { return mGeoMipTerrain != nullptr; }

    /**
     * 切换线框模式，用于观察 LOD 变换。
     * wireframe=true 时用线框渲染，=false 时恢复实心填充。
     */
    void SetWireframe(bool wireframe);

    bool IsWireframe() const { return mWireframe; }

private:
    GeoMipTerrainPtr mGeoMipTerrain;
    MaterialPtr      mMaterial;
    bool             mWireframe = false;
};

typedef std::shared_ptr<TerrainComponent> TerrainComponentPtr;

NS_RENDERSYSTEM_END

#endif // GNXENGINE_TERRAIN_COMPONENT_INCLUDE_H
