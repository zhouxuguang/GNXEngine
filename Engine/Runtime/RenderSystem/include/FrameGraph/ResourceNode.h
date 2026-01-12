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

	[[nodiscard]] auto getResourceId() const { return m_resourceId; }
	[[nodiscard]] auto getVersion() const { return m_version; }

private:
	ResourceNode(const std::string_view name, uint32_t nodeId, uint32_t resourceId, uint32_t version)
		: GraphNode{ name, nodeId }, m_resourceId{ resourceId }, m_version{ version } 
	{
	}

private:
	// Index to virtual resource (FrameGraph::m_resourceRegistry).
	const uint32_t m_resourceId;
	const uint32_t m_version;

	PassNode* m_producer = nullptr;
	PassNode* m_last = nullptr;
};

NS_RENDERSYSTEM_END
#endif
