#ifndef GNXENGINE_RESOURCENODE_INCLUDE_SHJFGJDSHGH
#define GNXENGINE_RESOURCENODE_INCLUDE_SHJFGJDSHGH

#include "DependencyGraph.h"
#include "PassNode.h"

NAMESPACE_GNXENGINE_BEGIN

class FrameGraph;
class ResourceEdgeBase;

class ResourceNode : public DependencyGraph::Node 
{
public:
    ResourceNode(FrameGraph& fg, FrameGraphHandle h, FrameGraphHandle parent) noexcept;
    ~ResourceNode() noexcept override;

    ResourceNode(ResourceNode const&) = delete;
    ResourceNode& operator=(ResourceNode const&) = delete;

    void addOutgoingEdge(ResourceEdgeBase* edge) noexcept;
    void setIncomingEdge(ResourceEdgeBase* edge) noexcept;

    // constants
    const FrameGraphHandle resourceHandle;


    // is a PassNode writing to this ResourceNode
    bool hasWriterPass() const noexcept 
    {
        return mWriterPass != nullptr;
    }

    // is any non culled Node (of any type) writing to this ResourceNode
    bool hasActiveWriters() const noexcept;

    // is the specified PassNode writing to this resource, if so return the corresponding edge.
    ResourceEdgeBase* getWriterEdgeForPass(PassNode const* node) const noexcept;
    bool hasWriteFrom(PassNode const* node) const noexcept;


    // is at least one PassNode reading from this ResourceNode
    bool hasReaders() const noexcept 
    {
        return !mReaderPasses.empty();
    }

    // is any non culled Node (of any type) reading from this ResourceNode
    bool hasActiveReaders() const noexcept;

    // is the specified PassNode reading this resource, if so return the corresponding edge.
    ResourceEdgeBase* getReaderEdgeForPass(PassNode const* node) const noexcept;


    void resolveResourceUsage(DependencyGraph& graph) noexcept;

    // return the parent's handle
    FrameGraphHandle getParentHandle() noexcept 
    {
        return mParentHandle;
    }

    // return the parent's node
    ResourceNode* getParentNode() noexcept;

    // return the oldest ancestor node
    static ResourceNode* getAncestorNode(ResourceNode* node) noexcept;

    // this is the parent resource we're reading from, as a propagating effect of
    // us being read from.
    void setParentReadDependency(ResourceNode* parent) noexcept;

    // this is the parent resource we're writing to, as a propagating effect of
    // us being writen to.
    void setParentWriteDependency(ResourceNode* parent) noexcept;

    void setForwardResourceDependency(ResourceNode* source) noexcept;

    // virtuals from DependencyGraph::Node
    char const* getName() const noexcept override;

    static FrameGraphHandle getHandle(ResourceNode const* node) noexcept {
        return node ? node->resourceHandle : FrameGraphHandle{};
    }

private:
    FrameGraph& mFrameGraph;
    std::vector<ResourceEdgeBase *> mReaderPasses;
    ResourceEdgeBase* mWriterPass = nullptr;
    FrameGraphHandle mParentHandle;
    DependencyGraph::Edge* mParentReadEdge = nullptr;
    DependencyGraph::Edge* mParentWriteEdge = nullptr;
    DependencyGraph::Edge* mForwardedEdge = nullptr;

    // virtuals from DependencyGraph::Node
    std::string graphvizify() const noexcept override;
    std::string graphvizifyEdgeColor() const noexcept override;
};

NAMESPACE_GNXENGINE_END

#endif
