#ifndef GNXENGINE_RESOURCE_INCLUDE_JDJHVJHJDHHDFJ
#define GNXENGINE_RESOURCE_INCLUDE_JDJHVJHJDHHDFJ

#include "DependencyGraph.h"

NAMESPACE_GNXENGINE_BEGIN

class PassNode;
class ResourceNode;
class ImportedRenderTarget;

/*
 * ResourceEdgeBase only exists to enforce type safety
 */
class ResourceEdgeBase : public DependencyGraph::Edge 
{
public:
    using Edge::Edge;
};

/*
 * 通用虚拟资源
 */
class VirtualResource 
{
public:
    // constants
    VirtualResource* parent;
    std::string name;

    // computed during compile()
    uint32_t refcount = 0;
    PassNode* first = nullptr;  // pass that needs to instantiate the resource
    PassNode* last = nullptr;   // pass that can destroy the resource

    explicit VirtualResource(const std::string& name) noexcept : parent(this), name(name) 
    {
    }

    VirtualResource(VirtualResource* parent, const std::string& name) noexcept : parent(parent), name(name) 
    {
    }

    VirtualResource(VirtualResource const& rhs) noexcept = delete;
    VirtualResource& operator=(VirtualResource const&) = delete;
    virtual ~VirtualResource() noexcept;

    // updates first/last/refcount
    void neededByPass(PassNode* pNode) noexcept;

    bool isSubResource() const noexcept { return parent != this; }

    VirtualResource* getResource() noexcept 
    {
        VirtualResource* p = this;
        while (p->parent != p) 
        {
            p = p->parent;
        }
        return p;
    }

    /*
     * Called during FrameGraph::compile(), this gives an opportunity for this resource to
     * calculate its effective usage flags.
     */
    virtual void resolveUsage(DependencyGraph& graph,
        ResourceEdgeBase const* const* edges, size_t count,
        ResourceEdgeBase const* writer) noexcept = 0;

    /* Instantiate the concrete resource */
    //virtual void devirtualize(ResourceCreationContext const& context) noexcept = 0;

    /* Destroy the concrete resource */
    //virtual void destroy(ResourceCreationContext const& context) noexcept = 0;

    /* Destroy an Edge instantiated by this resource */
    virtual void destroyEdge(DependencyGraph::Edge* edge) noexcept = 0;

    virtual std::string usageString() const noexcept = 0;

    virtual bool isImported() const noexcept { return false; }

    // this is to work around our lack of RTTI -- otherwise we could use dynamic_cast
    virtual ImportedRenderTarget* asImportedRenderTarget() noexcept { return nullptr; }

protected:
    void addOutgoingEdge(ResourceNode* node, ResourceEdgeBase* edge) noexcept;
    void setIncomingEdge(ResourceNode* node, ResourceEdgeBase* edge) noexcept;
    // these exist only so we don't have to include PassNode.h or ResourceNode.h
    static DependencyGraph::Node* toDependencyGraphNode(ResourceNode* node) noexcept;
    static DependencyGraph::Node* toDependencyGraphNode(PassNode* node) noexcept;
    static ResourceEdgeBase* getReaderEdgeForPass(ResourceNode* resourceNode, PassNode* passNode) noexcept;
    static ResourceEdgeBase* getWriterEdgeForPass(ResourceNode* resourceNode, PassNode* passNode) noexcept;
};

// ------------------------------------------------------------------------------------------------

#if 0



/*
 * Resource specific parts of a VirtualResource
 */
template<typename RESOURCE>
class Resource : public VirtualResource 
{
    using Usage = RESOURCE::Usage;

public:
    using Descriptor = RESOURCE::Descriptor;
    using SubResourceDescriptor = RESOURCE::SubResourceDescriptor;

    // valid only after devirtualize() has been called
    RESOURCE resource{};

    // valid only after resolveUsage() has been called
    Usage usage{};

    // our concrete (sub)resource descriptors -- used to create it.
    Descriptor descriptor;
    SubResourceDescriptor subResourceDescriptor;

    // whether the resource was detached
    bool detached = false;

    // An Edge with added data from this resource
    class ResourceEdge : public ResourceEdgeBase 
    {
    public:
        Usage usage;
        ResourceEdge(DependencyGraph& graph,
            DependencyGraph::Node* from, DependencyGraph::Node* to, Usage usage) noexcept
            : ResourceEdgeBase(graph, from, to), usage(usage) {
        }
    };

    Resource(const std::string& name, Descriptor const& desc) noexcept
        : VirtualResource(name), descriptor(desc) 
    {
    }

    Resource(Resource* parent, const std::string& name, SubResourceDescriptor const& desc) noexcept
        : VirtualResource(parent, name),
        //descriptor(RESOURCE::generateSubResourceDescriptor(parent->descriptor, desc)),
        subResourceDescriptor(desc) 
    {
    }

    ~Resource() noexcept override = default;

    // pass Node to resource Node edge (a write to)
    virtual bool connect(DependencyGraph& graph, PassNode* passNode, ResourceNode* resourceNode, Usage u) 
    {
        // TODO: we should check that usage flags are correct (e.g. a write flag is not used for reading)
        ResourceEdge* edge = static_cast<ResourceEdge*>(getWriterEdgeForPass(resourceNode, passNode));
        if (edge) 
        {
            edge->usage |= u;
        }
        else 
        {
            edge = new ResourceEdge(graph, toDependencyGraphNode(passNode), toDependencyGraphNode(resourceNode), u);
            setIncomingEdge(resourceNode, edge);
        }
        return true;
    }

    // resource Node to pass Node edge (a read from)
    virtual bool connect(DependencyGraph& graph, ResourceNode* resourceNode, PassNode* passNode, Usage u) 
    {
        // TODO: we should check that usage flags are correct (e.g. a write flag is not used for reading)
        // if passNode is already a reader of resourceNode, then just update the usage flags
        ResourceEdge* edge = static_cast<ResourceEdge*>(getReaderEdgeForPass(resourceNode, passNode));
        if (edge) 
        {
            edge->usage |= u;
        }
        else 
        {
            edge = new ResourceEdge(graph, toDependencyGraphNode(resourceNode), toDependencyGraphNode(passNode), u);
            addOutgoingEdge(resourceNode, edge);
        }
        return true;
    }

protected:
    /*
     * The virtual below must be in a header file as RESOURCE is only known at compile time
     */

    void resolveUsage(DependencyGraph& graph, ResourceEdgeBase const* const* edges,
        size_t const count, ResourceEdgeBase const* writer) noexcept override 
    {
        for (size_t i = 0; i < count; i++) 
        {
            if (graph.isEdgeValid(edges[i])) 
            {
                // this Edge is guaranteed to be a ResourceEdge<RESOURCE> by construction
                ResourceEdge const* const edge = static_cast<ResourceEdge const*>(edges[i]);
                usage |= edge->usage;
            }
        }

        // here don't check for the validity of Edge because even if the edge is invalid
        // the fact that we're called (not culled) means we need to take it into account
        // e.g. because the resource could be needed in a render target
        if (writer) 
        {
            ResourceEdge const* const edge = static_cast<ResourceEdge const*>(writer);
            usage |= edge->usage;
        }

        // propagate usage bits to the parents
        Resource* p = this;
        while (p != p->parent)
        {
            p = static_cast<Resource*>(p->parent);
            p->usage |= usage;
        }
    }

    void destroyEdge(DependencyGraph::Edge* edge) noexcept override 
    {
        // this Edge is guaranteed to be a ResourceEdge<RESOURCE> by construction
        delete static_cast<ResourceEdge*>(edge);
    }

    //void devirtualize(ResourceCreationContext const& context) noexcept override 
    //{
    //    if (!isSubResource()) 
    //    {
    //        ResourceAllocator<RESOURCE>::create(resource, context, name, descriptor, usage);
    //    }
    //    else 
    //    {
    //        // resource is guaranteed to be initialized before we are by construction
    //        resource = static_cast<Resource const*>(parent)->resource;
    //    }
    //}

    //void destroy(ResourceCreationContext const& context) noexcept override 
    //{
    //    if (detached || isSubResource()) 
    //    {
    //        return;
    //    }
    //    ResourceAllocator<RESOURCE>::destroy(resource, context);
    //}

    std::string usageString() const noexcept override 
    {
        return std::to_string(usage);
    }
};

/*
 * An imported resource is just like a regular one, except that it's constructed directly from
 * the concrete resource and it, evidently, doesn't create/destroy the concrete resource.
 */
template<typename RESOURCE>
class ImportedResource : public Resource<RESOURCE> 
{
public:
    using Descriptor = RESOURCE::Descriptor;
    using Usage = RESOURCE::Usage;

    ImportedResource(const std::string& name, Descriptor const& desc, Usage usage, RESOURCE const& rsrc) noexcept
        : Resource<RESOURCE>(name, desc) 
    {
        this->resource = rsrc;
        this->usage = usage;
    }

protected:
    //void devirtualize(ResourceCreationContext const&) noexcept override 
    //{
    //    // imported resources don't need to devirtualize
    //}
    //void destroy(ResourceCreationContext const&) noexcept override 
    //{
    //    // imported resources never destroy the concrete resource
    //}

    bool isImported() const noexcept override { return true; }

    bool connect(DependencyGraph& graph, PassNode* passNode, ResourceNode* resourceNode, FrameGraphTexture::Usage u) override 
    {
        assertConnect(u);
        return Resource<RESOURCE>::connect(graph, passNode, resourceNode, u);
    }
    
    bool connect(DependencyGraph& graph, ResourceNode* resourceNode, PassNode* passNode, FrameGraphTexture::Usage u) override 
    {
        assertConnect(u);
        return Resource<RESOURCE>::connect(graph, resourceNode, passNode, u);
    }

private:
    /*void assertConnect(FrameGraphTexture::Usage u) const 
    {
        FILAMENT_CHECK_PRECONDITION((u & this->usage) == u)
            << "Requested usage " << utils::to_string(u).c_str()
            << " not available on imported resource \"" << this->name.c_str() << "\" with usage "
            << utils::to_string(this->usage).c_str();
    }*/
};


//class ImportedRenderTarget final : public ImportedResource<FrameGraphTexture> {
//public:
//    backend::Handle<backend::HwRenderTarget> target;
//    FrameGraphRenderPass::ImportDescriptor importedDesc;
//
//    UTILS_NOINLINE
//        ImportedRenderTarget(utils::StaticString name,
//            FrameGraphTexture::Descriptor const& mainAttachmentDesc,
//            FrameGraphRenderPass::ImportDescriptor const& importedDesc,
//            backend::Handle<backend::HwRenderTarget> target);
//
//    ~ImportedRenderTarget() noexcept override;
//
//protected:
//    UTILS_NOINLINE
//        bool connect(DependencyGraph& graph,
//            PassNode* passNode, ResourceNode* resourceNode, FrameGraphTexture::Usage u) override;
//
//    UTILS_NOINLINE
//        bool connect(DependencyGraph& graph,
//            ResourceNode* resourceNode, PassNode* passNode, FrameGraphTexture::Usage u) override;
//
//    ImportedRenderTarget* asImportedRenderTarget() noexcept override { return this; }
//
//private:
//    void assertConnect(FrameGraphTexture::Usage u) const;
//
//    static FrameGraphTexture::Usage usageFromAttachmentsFlags(
//        backend::TargetBufferFlags attachments) noexcept;
//};

// ------------------------------------------------------------------------------------------------

// prevent implicit instantiation of Resource<FrameGraphTexture> which is a known type
//extern template class Resource<FrameGraphTexture>;
//extern template class ImportedResource<FrameGraphTexture>;

#endif

NAMESPACE_GNXENGINE_END

#endif