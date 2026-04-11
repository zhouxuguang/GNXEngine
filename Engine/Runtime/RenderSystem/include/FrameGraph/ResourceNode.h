#ifndef GNXENGINE_RENDERSYSYTEM_FRAMEGRAPH_RESOURCENODE_H
#define GNXENGINE_RENDERSYSYTEM_FRAMEGRAPH_RESOURCENODE_H

#include "GraphNode.h"

NS_RENDERSYSTEM_BEGIN

class PassNode;

// 资源节点，包括纹理和buffer
class ResourceNode final : public GraphNode
{
	friend class FrameGraph;

public:
	ResourceNode(const ResourceNode&) = delete;
	ResourceNode(ResourceNode&&) noexcept = default;

	ResourceNode& operator=(const ResourceNode&) = delete;
	ResourceNode& operator=(ResourceNode&&) noexcept = delete;

	[[nodiscard]] auto getResourceId() const { return mResourceId; }
	[[nodiscard]] auto getVersion() const { return mVersion; }

private:
	ResourceNode(const std::string_view name, uint32_t nodeId, uint32_t resourceId, uint32_t version)
		: GraphNode{ name, nodeId }, mResourceId{ resourceId }, mVersion{ version } 
	{
	}

private:
	// Index to virtual resource (FrameGraph::mResourceRegistry).
	const uint32_t mResourceId;
	const uint32_t mVersion;

	PassNode* mProducer = nullptr;
	PassNode* mLast = nullptr;
};

NS_RENDERSYSTEM_END

#endif
