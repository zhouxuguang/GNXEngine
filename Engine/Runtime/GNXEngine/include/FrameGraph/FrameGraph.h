#pragma once

#include "PassNode.h"
#include "ResourceNode.h"
#include "ResourceEntry.h"

NAMESPACE_GNXENGINE_BEGIN

class GNXENGINE_API FrameGraph
{
    friend class FrameGraphPassResources;

public:
    FrameGraph() = default;
    FrameGraph(const FrameGraph&) = delete;
    FrameGraph(FrameGraph&&) noexcept = delete;

    FrameGraph& operator=(const FrameGraph&) = delete;
    FrameGraph& operator=(FrameGraph&&) noexcept = delete;

    friend std::ostream& operator<<(std::ostream&, const FrameGraph&);

    static constexpr auto kFlagsIgnored = ~0;

    class Builder final
    {
        friend class FrameGraph;

    public:
        Builder() = delete;
        Builder(const Builder&) = delete;
        Builder(Builder&&) noexcept = delete;

        Builder& operator=(const Builder&) = delete;
        Builder& operator=(Builder&&) noexcept = delete;

        template <_VIRTUALIZABLE_CONCEPT(T)>
        /** Declares the creation of a resource. */
        [[nodiscard]] FrameGraphResource Create(const std::string_view name, const typename T::Desc&);
        
        /** Declares read operation. */
        FrameGraphResource Read(FrameGraphResource id, uint32_t flags = kFlagsIgnored);
        /**
         * Declares write operation.
         * @remark Writing to imported resource counts as side-effect.
         */
        [[nodiscard]] FrameGraphResource Write(FrameGraphResource id, uint32_t flags = kFlagsIgnored);

        /** Ensures that this pass is not culled during the compilation phase. */
        Builder& setSideEffect() 
        {
            m_passNode.mHasSideEffect = true;
            return *this;
        }
        
        Builder& EnableAsyncCompute(bool enableAsyncCompute)
        {
            m_passNode.mEnableAsyncCompute = enableAsyncCompute;
            return *this;
        }

    private:
        Builder(FrameGraph& fg, PassNode& node) : m_frameGraph(fg), m_passNode(node)
        {
        }

    private:
        FrameGraph& m_frameGraph;
        PassNode& m_passNode;
    };

    void Reserve(uint32_t numPasses, uint32_t numResources);

    struct NoData {};
    /**
     * @param setup  pass配置的回调，在当前函数进行回调，回调的声明如下：
     *              (FrameGraph::Builder &builder, PassData &data)
     * @param exec 在Execute函数中执行的回调，回调的声明如下：
     *              (const PassData &data, FrameGraphPassResources &resources, void *)
     *
     */
    template <typename Data = NoData, typename Setup, typename Execute>
    const Data& AddPass(const std::string_view name, Setup&& setup, Execute&& exec)
    {
        static_assert(std::is_invocable_v<Setup, Builder&, Data&>, "Invalid setup callback");
        static_assert(std::is_invocable_v<Execute, const Data&, FrameGraphPassResources&, void*>, "Invalid exec callback");
        static_assert(sizeof(Execute) < 1024, "Execute captures too much");

        auto* pass = new(std::nothrow) FrameGraphPass<Data, Execute>(std::forward<Execute>(exec));
        auto& passNode = _createPassNode(name, std::unique_ptr<FrameGraphPass<Data, Execute>>(pass));
        Builder builder{*this, passNode};
        std::invoke(setup, builder, pass->data);
        return pass->data;
    }

    template <_VIRTUALIZABLE_CONCEPT(T)>
    [[nodiscard]] const typename T::Desc& GetDescriptor(FrameGraphResource id) const
    {
        return _getResourceEntry(id).getDescriptor<T>();
    }

    template <_VIRTUALIZABLE_CONCEPT(T)>
    /** Imports the given resource T into FrameGraph. */
    [[nodiscard]] FrameGraphResource Import(const std::string_view name, const typename T::Desc&, T&&);

    /** @return True if the given resource is valid for read/write operation. */
    [[nodiscard]] bool IsValid(FrameGraphResource id) const;

    /** 剔除无用的资源和Pass*/
    void Compile();
    
    /** 执行Pass的回调 */
    void Execute(void* context = nullptr, void* allocator = nullptr);

    template <typename Writer>
    std::ostream& DebugOutput(std::ostream&, Writer&&) const;

private:
    [[nodiscard]] PassNode& _createPassNode(const std::string_view name, std::unique_ptr<FrameGraphPassConcept>&&);

    template <_VIRTUALIZABLE_CONCEPT(T)>
    [[nodiscard]] FrameGraphResource _create(const ResourceEntry::Type,
        const std::string_view name,
        const typename T::Desc&, T&&);

    [[nodiscard]] ResourceNode&
        _createResourceNode(const std::string_view name, uint32_t resourceId,
            uint32_t version = ResourceEntry::kInitialVersion);
    /** Increments ResourceEntry version and produces a renamed handle. */
    [[nodiscard]] FrameGraphResource _clone(FrameGraphResource id);

    [[nodiscard]] const ResourceNode& _getResourceNode(FrameGraphResource id) const;
    [[nodiscard]] const ResourceEntry& _getResourceEntry(FrameGraphResource id) const;
    [[nodiscard]] const ResourceEntry& _getResourceEntry(const ResourceNode&) const;

    [[nodiscard]] decltype(auto) _getResourceNode(FrameGraphResource id) 
    {
        return const_cast<ResourceNode&>(
            const_cast<const FrameGraph*>(this)->_getResourceNode(id));
    }

    [[nodiscard]] decltype(auto) _getResourceEntry(FrameGraphResource id) 
    {
        return const_cast<ResourceEntry&>(
            const_cast<const FrameGraph*>(this)->_getResourceEntry(id));
    }

    [[nodiscard]] decltype(auto) _getResourceEntry(const ResourceNode& node) 
    {
        return const_cast<ResourceEntry&>(
            const_cast<const FrameGraph*>(this)->_getResourceEntry(node));
    }

private:
    std::vector<PassNode> m_passNodes;
    std::vector<ResourceNode> m_resourceNodes;
    std::vector<ResourceEntry> m_resourceRegistry;
};

class FrameGraphPassResources
{
    friend class FrameGraph;

public:
    FrameGraphPassResources() = delete;
    FrameGraphPassResources(const FrameGraphPassResources&) = delete;
    FrameGraphPassResources(FrameGraphPassResources&&) noexcept = delete;
    ~FrameGraphPassResources() = default;

    FrameGraphPassResources& operator=(const FrameGraphPassResources&) = delete;
    FrameGraphPassResources& operator=(FrameGraphPassResources&&) noexcept = delete;

    /**
     * @note Causes runtime-error with:
     * - Attempt to use obsolete handle (the one that has been renamed before)
     * - Incorrect resource type T
     */
    template <_VIRTUALIZABLE_CONCEPT(T)>
    [[nodiscard]] T& Get(FrameGraphResource id);
    
    template <_VIRTUALIZABLE_CONCEPT(T)>
    [[nodiscard]] const typename T::Desc& GetDescriptor(FrameGraphResource id) const;

private:
    FrameGraphPassResources(FrameGraph& fg, const PassNode& node) : m_frameGraph(fg), m_passNode(node)
    {
    }

private:
    FrameGraph& m_frameGraph;
    const PassNode& m_passNode;
};


// 实现部分

template <_VIRTUALIZABLE_CONCEPT_IMPL(T)>
inline FrameGraphResource FrameGraph::Import(const std::string_view name, const typename T::Desc& desc, T&& resource) 
{
    return _create<T>(ResourceEntry::Type::Imported, name, desc, std::forward<T>(resource));
}

template <typename Writer>
inline std::ostream& FrameGraph::DebugOutput(std::ostream& os, Writer&& writer) const
{
    for (const auto& node : m_passNodes)
    {
        writer(node, m_resourceNodes);
    }
    for (const auto& node : m_resourceNodes)
    {
        writer(node, m_resourceRegistry[node.m_resourceId], m_passNodes);
    }
    writer.flush(os);
    return os;
}

//
// (private):
//

template <_VIRTUALIZABLE_CONCEPT_IMPL(T)>
inline FrameGraphResource
FrameGraph::_create(const ResourceEntry::Type type, const std::string_view name, const typename T::Desc& desc, T&& resource)
{
    const auto resourceId = static_cast<uint32_t>(m_resourceRegistry.size());
    m_resourceRegistry.emplace_back(
        ResourceEntry{ type, resourceId, desc, std::forward<T>(resource) });
    return _createResourceNode(name, resourceId).getId();
}

//
// FrameGraph::Builder class:
//

template <_VIRTUALIZABLE_CONCEPT_IMPL(T)>
inline FrameGraphResource
FrameGraph::Builder::Create(const std::string_view name, const typename T::Desc& desc)
{
    const auto id = m_frameGraph._create<T>(ResourceEntry::Type::Transient, name, desc, T{});
    return m_passNode.m_creates.emplace_back(id);
}

//
// FrameGraphPassResources class:
//

template <_VIRTUALIZABLE_CONCEPT_IMPL(T)>
inline T& FrameGraphPassResources::Get(FrameGraphResource id)
{
    assert(m_passNode.reads(id) || m_passNode.creates(id) || m_passNode.writes(id));
    return m_frameGraph._getResourceEntry(id).get<T>();
}

template <_VIRTUALIZABLE_CONCEPT_IMPL(T)>
inline const typename T::Desc&
FrameGraphPassResources::GetDescriptor(FrameGraphResource id) const
{
    assert(m_passNode.reads(id) || m_passNode.creates(id) || m_passNode.writes(id));
    return m_frameGraph.GetDescriptor<T>(id);
}

NAMESPACE_GNXENGINE_END
