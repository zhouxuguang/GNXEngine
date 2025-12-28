#include "FrameGraph/PassNode.h"
#include "FrameGraph/FrameGraph.h"

NAMESPACE_GNXENGINE_BEGIN

PassNode::PassNode(FrameGraph& fg) noexcept
        : Node(fg.getGraph()),
          mFrameGraph(fg)//,
//          devirtualize(fg.getArena()),
//          destroy(fg.getArena()) 
{
}

PassNode::PassNode(PassNode&& rhs) noexcept = default;

PassNode::~PassNode() noexcept = default;

std::string PassNode::graphvizifyEdgeColor() const noexcept
{
    return "red";
}

void PassNode::registerResource(FrameGraphHandle const resourceHandle) noexcept 
{
//    VirtualResource* resource = mFrameGraph.getResource(resourceHandle);
//    resource->neededByPass(this);
    mDeclaredHandles.insert(resourceHandle.index);
}

NAMESPACE_GNXENGINE_END
