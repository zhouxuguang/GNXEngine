#ifndef GNXENGINE_RENDERSYSYTEM_FRAMEGRAPH_FRAMEGRAPHHELPER_H
#define GNXENGINE_RENDERSYSYTEM_FRAMEGRAPH_FRAMEGRAPHHELPER_H

#include "GraphNode.h"
#include "Runtime/RenderCore/include/ComputeBuffer.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include <string_view>

NS_RENDERSYSTEM_BEGIN

class FrameGraph;
class FrameGraphPassResources;

[[nodiscard]] RENDERSYSTEM_API FrameGraphResource ImportTexture(FrameGraph& fg, const std::string_view name, RenderCore::RCTexturePtr texture);

[[nodiscard]] RENDERSYSTEM_API RenderCore::RCTexturePtr GetTexture(FrameGraphPassResources& resources, FrameGraphResource id);

[[nodiscard]] RENDERSYSTEM_API FrameGraphResource ImportBuffer(FrameGraph& fg, const std::string_view name, RenderCore::ComputeBufferPtr buffer);

[[nodiscard]] RENDERSYSTEM_API RenderCore::ComputeBufferPtr GetBuffer(FrameGraphPassResources& resources, FrameGraphResource id);

NS_RENDERSYSTEM_END

#endif

