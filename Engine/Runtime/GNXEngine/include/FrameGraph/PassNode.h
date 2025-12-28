#ifndef GNXENGINE_PASSNODE_INCLUDESDGKGJND
#define GNXENGINE_PASSNODE_INCLUDESDGKGJND

#include "DependencyGraph.h"

NAMESPACE_GNXENGINE_BEGIN

class FrameGraph;

/** A handle on a resource */
class FrameGraphHandle {
public:
    using Index = uint16_t;
    using Version = uint16_t;

private:
    template<typename T>
    friend class FrameGraphId;

    friend class Blackboard;
    friend class FrameGraph;
    friend class FrameGraphResources;
    friend class PassNode;
    friend class ResourceNode;

    // private ctor -- this cannot be constructed by users
    FrameGraphHandle() noexcept = default;
    explicit FrameGraphHandle(Index const index) noexcept : index(index) {}

    // index to the resource handle
    static constexpr uint16_t UNINITIALIZED = std::numeric_limits<Index>::max();
    uint16_t index = UNINITIALIZED;     // index to a ResourceSlot
    Version version = 0;

public:
    FrameGraphHandle(FrameGraphHandle const& rhs) noexcept = default;

    FrameGraphHandle& operator=(FrameGraphHandle const& rhs) noexcept = default;

    bool isInitialized() const noexcept { return index != UNINITIALIZED; }

    operator bool() const noexcept { return isInitialized(); }

    void clear() noexcept { index = UNINITIALIZED; version = 0; }

    bool operator < (const FrameGraphHandle& rhs) const noexcept {
        return index < rhs.index;
    }

    bool operator == (const FrameGraphHandle& rhs) const noexcept {
        return (index == rhs.index);
    }

    bool operator != (const FrameGraphHandle& rhs) const noexcept {
        return !operator==(rhs);
    }
};

class PassNode : public DependencyGraph::Node 
{
protected:
    friend class FrameGraphResources;
    FrameGraph& mFrameGraph;
    std::unordered_set<FrameGraphHandle::Index> mDeclaredHandles;
public:
    explicit PassNode(FrameGraph& fg) noexcept;
    PassNode(PassNode&& rhs) noexcept;
    PassNode(PassNode const&) = delete;
    PassNode& operator=(PassNode const&) = delete;
    ~PassNode() noexcept override;
    using NodeID = DependencyGraph::NodeID;

    void registerResource(FrameGraphHandle resourceHandle) noexcept;

    virtual void execute(/* FrameGraphResources const& resources, backend::DriverApi& driver*/) noexcept = 0;
    virtual void resolve() noexcept = 0;
    std::string graphvizifyEdgeColor() const noexcept override;

    //Vector<VirtualResource*> devirtualize;         // resources we need to create before executing
    //Vector<VirtualResource*> destroy;              // resources we need to destroy after executing
};

NAMESPACE_GNXENGINE_END

#endif

