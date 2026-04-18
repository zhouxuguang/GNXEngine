//
//  TerrainGenerator.h
//  GNXEngine
//
//  CPU-side terrain mesh generation from procedural heightmap.
//  Phase 1 of the terrain system - heightmap driven terrain.
//

#ifndef GNXENGINE_TERRAIN_GENERATOR_INCLUDE_H
#define GNXENGINE_TERRAIN_GENERATOR_INCLUDE_H

#include "../RSDefine.h"
#include "../mesh/Mesh.h"

NS_RENDERSYSTEM_BEGIN

class RENDERSYSTEM_API TerrainGenerator
{
public:
    /**
     * Generate a terrain mesh from procedural multi-octave noise.
     * @param resolution    Grid resolution (resolution x resolution vertices)
     * @param worldSizeXZ   Total world-space size of the terrain in XZ plane
     * @param heightScale   Multiplier applied to raw noise height values
     * @return Shared pointer to the constructed Mesh (GPU buffers uploaded)
     */
    static MeshPtr GenerateMesh(uint32_t resolution = 256,
                                float worldSizeXZ = 512.0f,
                                float heightScale = 1.0f);

    /**
     * Generate a height-based diffuse texture (green -> brown -> gray -> white).
     * @param resolution    Texture resolution (should match mesh resolution)
     * @param worldSizeXZ   World size (must match the mesh generation)
     * @param heightScale   Height scale (must match the mesh generation)
     * @return GPU texture pointer (RGBA8, ShaderRead)
     */
    static RenderCore::RCTexture2DPtr GenerateDiffuseTexture(
        uint32_t resolution,
        float worldSizeXZ,
        float heightScale);

    /**
     * Generate a terrain mesh from a heightmap image file.
     * Supports both 8-bit (GRAY8) and 16-bit (GRAY16) grayscale PNGs.
     * The mesh grid resolution is determined by the heightmap image dimensions
     * (one vertex per pixel, no resampling).
     * @param heightmapPath Path to the heightmap image (PNG, grayscale)
     * @param worldSizeXZ   Total world-space size of the terrain in XZ plane
     * @param heightScale   Multiplier applied to normalized height values [0,1]
     * @return Shared pointer to the constructed Mesh (GPU buffers uploaded)
     */
    static MeshPtr GenerateMeshFromHeightMap(const char* heightmapPath,
                                             float worldSizeXZ = 512.0f,
                                             float heightScale = 80.0f);

    /**
     * Load a diffuse texture from an image file.
     * @param texturePath Path to the texture image (PNG, JPEG, etc.)
     * @return GPU texture pointer (RGBA8, ShaderRead)
     */
    static RenderCore::RCTexture2DPtr LoadDiffuseTexture(const char* texturePath);

private:
    /** Multi-octave sine-wave noise for natural-looking hills */
    static float ComputeHeight(float x, float z);
};

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_TERRAIN_GENERATOR_INCLUDE_H */
