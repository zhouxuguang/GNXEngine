#pragma once

#include "GraphNode.h"
#include "Runtime/RenderCore/include/ComputeBuffer.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include <string_view>

NAMESPACE_GNXENGINE_BEGIN

class FrameGraph;
class FrameGraphPassResources;

[[nodiscard]] FrameGraphResource ImportTexture(FrameGraph& fg, const std::string_view name, RenderCore::RCTexturePtr texture);

[[nodiscard]] RenderCore::RCTexturePtr GetTexture(FrameGraphPassResources& resources, FrameGraphResource id);

[[nodiscard]] FrameGraphResource ImportBuffer(FrameGraph& fg, const std::string_view name, RenderCore::ComputeBufferPtr buffer);

[[nodiscard]] RenderCore::ComputeBufferPtr GetBuffer(FrameGraphPassResources& resources, FrameGraphResource id);

NAMESPACE_GNXENGINE_END

