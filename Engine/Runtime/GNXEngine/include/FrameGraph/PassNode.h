#ifndef GNXENGINE_PASSNODE_INCLUDESDGKGJND
#define GNXENGINE_PASSNODE_INCLUDESDGKGJND

#include "DependencyGraph.h"

NAMESPACE_GNXENGINE_BEGIN

class FrameGraph;
class PassNode;
class ResourceNode;
class ImportedRenderTarget;

/** A handle on a resource */
class FrameGraphHandle
{
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

/** A typed handle on a resource */
template<typename RESOURCE>
class FrameGraphId : public FrameGraphHandle 
{
public:
	using FrameGraphHandle::FrameGraphHandle;
	FrameGraphId() noexcept = default;
	explicit FrameGraphId(FrameGraphHandle const r) : FrameGraphHandle(r) {}
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

	virtual void execute(/*FrameGraphResources const& resources, backend::DriverApi& driver*/) noexcept = 0;
	virtual void resolve() noexcept = 0;
	std::string graphvizifyEdgeColor() const noexcept override;

	//Vector<VirtualResource*> devirtualize;         // resources we need to create before executing
	//Vector<VirtualResource*> destroy;              // resources we need to destroy after executing
};

#if 0

class RenderPassNode : public PassNode
{
public:
	class RenderPassData
	{
	public:
		static constexpr size_t ATTACHMENT_COUNT = 8 + 2;
		std::string name{};
		FrameGraphRenderPass::Descriptor descriptor;
		bool imported = false;
		backend::TargetBufferFlags targetBufferFlags = {};
		FrameGraphId<FrameGraphTexture> attachmentInfo[ATTACHMENT_COUNT] = {};
		ResourceNode* incoming[ATTACHMENT_COUNT] = {};  // nodes of the incoming attachments
		ResourceNode* outgoing[ATTACHMENT_COUNT] = {};  // nodes of the outgoing attachments
		struct
		{
			backend::Handle<backend::HwRenderTarget> target;
			backend::RenderPassParams params;
		} backend;

		void devirtualize(FrameGraph& fg, TextureCacheInterface& textureCache) noexcept;
		void destroy(TextureCacheInterface& textureCache) const noexcept;
	};

	RenderPassNode(FrameGraph& fg, const char* name, FrameGraphPassBase* base) noexcept;
	RenderPassNode(RenderPassNode&& rhs) noexcept;
	~RenderPassNode() noexcept override;

	uint32_t declareRenderTarget(FrameGraph& fg, FrameGraph::Builder& builder,
		utils::StaticString name, FrameGraphRenderPass::Descriptor const& descriptor);

	RenderPassData const* getRenderPassData(uint32_t id) const noexcept;

private:
	// virtuals from DependencyGraph::Node
	char const* getName() const noexcept override { return mName; }
	utils::CString graphvizify() const noexcept override;
	void execute(FrameGraphResources const& resources, backend::DriverApi& driver) noexcept override;
	void resolve() noexcept override;

	// constants
	const char* const mName = nullptr;
	UniquePtr<FrameGraphPassBase, LinearAllocatorArena> mPassBase;

	// set during setup
	std::vector<RenderPassData> mRenderTargetData;
};

#endif


// 最终用于上屏的渲染节点
class PresentPassNode : public PassNode
{
public:
	explicit PresentPassNode(FrameGraph& fg) noexcept;
	PresentPassNode(PresentPassNode&& rhs) noexcept;
	~PresentPassNode() noexcept override;
	PresentPassNode(PresentPassNode const&) = delete;
	PresentPassNode& operator=(PresentPassNode const&) = delete;
	void execute(/*FrameGraphResources const& resources, backend::DriverApi& driver*/) noexcept override;
	void resolve() noexcept override;
private:
	// virtuals from DependencyGraph::Node
	char const* getName() const noexcept override;
	std::string graphvizify() const noexcept override;
};

NAMESPACE_GNXENGINE_END

#endif

