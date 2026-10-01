#ifndef GNXENGINE_DEFERRED_GEOMETRY_H
#define GNXENGINE_DEFERRED_GEOMETRY_H

#include "RSDefine.h"
#include "Runtime/RenderCore/include/RCBuffer.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include "Runtime/RenderCore/include/UniformBuffer.h"
#include "Runtime/RenderCore/include/TextureSampler.h"
#include <vector>

NS_RENDERSYSTEM_BEGIN

// A frame-local draw snapshot. Shared resource handles keep tile GPU resources
// alive through both passes even if the scene's LOD selection changes later.
struct DeferredGeometryDraw
{
    RenderCore::RCBufferPtr vertexBuffer;
    RenderCore::RCBufferPtr indexBuffer;
    RenderCore::RCTexturePtr baseColor;
    RenderCore::TextureSamplerPtr sampler;
    RenderCore::UniformBufferPtr objectUBO;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    // Render the sampled base color without direct or environment lighting.
    bool unlit = false;
};

class RENDERSYSTEM_API DeferredGeometryProvider
{
public:
    virtual ~DeferredGeometryProvider() = default;
    virtual void CollectDeferredGeometry(std::vector<DeferredGeometryDraw>& draws) = 0;
};

NS_RENDERSYSTEM_END

#endif
