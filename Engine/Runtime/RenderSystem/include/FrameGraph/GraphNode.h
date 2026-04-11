#ifndef GNXENGINE_RENDERSYSYTEM_FRAMEGRAPH_GRAPHNODE_H
#define GNXENGINE_RENDERSYSYTEM_FRAMEGRAPH_GRAPHNODE_H

#include <string>
#include <cstdint>
#include "../RSDefine.h"

NS_RENDERSYSTEM_BEGIN

class FrameGraph;

using FrameGraphResource = int32_t;

// frameGraph的节点，主要包括pass和资源
class RENDERSYSTEM_API GraphNode
{
	friend class FrameGraph;

public:
	GraphNode() = delete;
	GraphNode(const GraphNode&) = delete;
	GraphNode(GraphNode&&) noexcept = default;
	virtual ~GraphNode() = default;

	GraphNode& operator=(const GraphNode&) = delete;
	GraphNode& operator=(GraphNode&&) noexcept = delete;

	[[nodiscard]] auto getId() const { return mId; }
	[[nodiscard]] const std::string& getName() const { return mName; }
	[[nodiscard]] auto getRefCount() const { return mRefCount; }

protected:
	GraphNode(const std::string_view name, uint32_t id) : mName(name), mId(id)
	{
	}

private:
	std::string mName;
	// Unique id, matches an array index in FrameGraph.
	const uint32_t mId;
	int32_t mRefCount = 0;
};

NS_RENDERSYSTEM_END

#endif
