#include "FrameGraph/ResourceNode.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/Resource.h"

NAMESPACE_GNXENGINE_BEGIN

ResourceNode::ResourceNode(FrameGraph& fg, FrameGraphHandle const h, FrameGraphHandle const parent) noexcept
	: Node(fg.getGraph()),
	resourceHandle(h), mFrameGraph(fg),/*, mReaderPasses(fg.getArena()), */mParentHandle(parent)
{
}

ResourceNode::~ResourceNode() noexcept
{
	//    VirtualResource* resource = mFrameGraph.getResource(resourceHandle);
	//    assert(resource);
	//    resource->destroyEdge(mWriterPass);
	//    for (auto* pEdge : mReaderPasses) 
	//    {
	//        resource->destroyEdge(pEdge);
	//    }
	delete mParentReadEdge;
	delete mParentWriteEdge;
	delete mForwardedEdge;
}

ResourceNode* ResourceNode::getParentNode() noexcept
{
	//    ResourceNode* const parentNode = mParentHandle ?
	//            mFrameGraph.getActiveResourceNode(mParentHandle) : nullptr;
	//    assert(mParentHandle == ResourceNode::getHandle(parentNode));
	//    return parentNode;
	return nullptr;
}

ResourceNode* ResourceNode::getAncestorNode(ResourceNode* node) noexcept
{
	ResourceNode* ancestor = node;
	do
	{
		node = node->getParentNode();
		ancestor = node ? node : ancestor;
	} while (node);
	return ancestor;
}

char const* ResourceNode::getName() const noexcept
{
	return "";
	//return mFrameGraph.getResource(resourceHandle)->name.c_str();
}

void ResourceNode::addOutgoingEdge(ResourceEdgeBase* edge) noexcept
{
	mReaderPasses.push_back(edge);
}

void ResourceNode::setIncomingEdge(ResourceEdgeBase* edge) noexcept
{
	assert(mWriterPass == nullptr);
	mWriterPass = edge;
}

bool ResourceNode::hasActiveReaders() const noexcept
{
	// here we don't use mReaderPasses because this wouldn't account for subresources
	DependencyGraph& dependencyGraph = mFrameGraph.getGraph();
	auto const& readers = dependencyGraph.getOutgoingEdges(this);
	for (auto const& reader : readers)
	{
		if (!dependencyGraph.getNode(reader->to)->isCulled())
		{
			return true;
		}
	}
	return false;
}

bool ResourceNode::hasActiveWriters() const noexcept
{
	// here we don't use mReaderPasses because this wouldn't account for subresources
	DependencyGraph const& dependencyGraph = mFrameGraph.getGraph();
	auto const& writers = dependencyGraph.getIncomingEdges(this);
	// writers are not culled by definition if we're not culled ourselves
	return !writers.empty();
}

ResourceEdgeBase* ResourceNode::getReaderEdgeForPass(PassNode const* node) const noexcept
{
	auto pos = std::find_if(mReaderPasses.begin(), mReaderPasses.end(), [node](ResourceEdgeBase const* edge)
	{
		return edge->to == node->getId();
	});
	return pos != mReaderPasses.end() ? *pos : nullptr;
}

ResourceEdgeBase* ResourceNode::getWriterEdgeForPass(PassNode const* node) const noexcept
{
	return mWriterPass && mWriterPass->from == node->getId() ? mWriterPass : nullptr;
}

bool ResourceNode::hasWriteFrom(PassNode const* node) const noexcept
{
	return bool(getWriterEdgeForPass(node));
}

void ResourceNode::setParentReadDependency(ResourceNode* parent) noexcept
{
	if (!mParentReadEdge)
	{
		mParentReadEdge = new(std::nothrow) DependencyGraph::Edge(mFrameGraph.getGraph(), parent, this);
	}
}

void ResourceNode::setParentWriteDependency(ResourceNode* parent) noexcept
{
	if (!mParentWriteEdge)
	{
		mParentWriteEdge = new(std::nothrow) DependencyGraph::Edge(mFrameGraph.getGraph(), this, parent);
	}
}

void ResourceNode::setForwardResourceDependency(ResourceNode* source) noexcept
{
	assert(!mForwardedEdge);
	mForwardedEdge = new(std::nothrow) DependencyGraph::Edge(mFrameGraph.getGraph(), this, source);
}

void ResourceNode::resolveResourceUsage(DependencyGraph& graph) noexcept
{
	/*    VirtualResource* pResource = mFrameGraph.getResource(resourceHandle);
	    assert_invariant(pResource);
	    if (pResource->refcount) 
	    {
	        pResource->resolveUsage(graph, mReaderPasses.data(), mReaderPasses.size(), mWriterPass);
	    }*/
}

std::string ResourceNode::graphvizify() const noexcept
{
#ifndef NDEBUG
	std::string s;

#if 0

	uint32_t const id = getId();
	const char* const nodeName = getName();
	VirtualResource const* const pResource = mFrameGraph.getResource(resourceHandle);
	FrameGraph::ResourceSlot const& slot = mFrameGraph.getResourceSlot(resourceHandle);

	s.append("[label=\"");
	s.append(nodeName);
	s.append("\\nrefs: ");
	s.append(std::to_string(pResource->refcount));
	s.append(", id: ");
	s.append(std::to_string(id));
	s.append("\\nversion: ");
	s.append(std::to_string(resourceHandle.version));
	s.append("/");
	s.append(std::to_string(slot.version));
	if (pResource->isImported())
	{
		s.append(", imported");
	}
	s.append("\\nusage: ");
	s.append(pResource->usageString().c_str());
	s.append("\", ");

	s.append("style=filled, fillcolor=");
	s.append(pResource->refcount ? "skyblue" : "skyblue4");
	s.append("]");

#endif

	return s;
#else
	return {};
#endif
}

std::string ResourceNode::graphvizifyEdgeColor() const noexcept
{
	return "darkolivegreen";
}

NAMESPACE_GNXENGINE_END
