#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/FrameGraphBlackboard.h"
#include "GraphvizWriter.h"
#include <stack>

NS_RENDERSYSTEM_BEGIN

//
// FrameGraph class:
//

void FrameGraph::Reserve(uint32_t numPasses, uint32_t numResources)
{
	mPassNodes.reserve(numPasses);
	mResourceNodes.reserve(numResources);
	mResourceRegistry.reserve(numResources);
}

bool FrameGraph::IsValid(FrameGraphResource id) const
{
	const auto& node = _getResourceNode(id);
	return node.getVersion() == _getResourceEntry(node).getVersion();
}

void FrameGraph::Compile()
{
	for (auto& pass : mPassNodes)
	{
		pass.mRefCount = static_cast<int32_t>(pass.mWrites.size());
		for (const auto [id, _] : pass.mReads)
		{
			auto& consumed = mResourceNodes[id];
			consumed.mRefCount++;
		}
		for (const auto [id, _] : pass.mWrites)
		{
			auto& written = mResourceNodes[id];
			written.mProducer = &pass;
		}
	}

	// -- Culling:

	std::stack<ResourceNode*> unreferencedResources;
	for (auto& node : mResourceNodes)
	{
		if (node.mRefCount == 0) unreferencedResources.push(&node);
	}
	while (!unreferencedResources.empty())
	{
		auto* unreferencedResource = unreferencedResources.top();
		unreferencedResources.pop();
		PassNode* producer{ unreferencedResource->mProducer };
		if (producer == nullptr || producer->hasSideEffect()) continue;

		assert(producer->mRefCount >= 1);
		if (--producer->mRefCount == 0)
		{
			for (const auto [id, _] : producer->mReads)
			{
				auto& node = mResourceNodes[id];
				if (--node.mRefCount == 0) unreferencedResources.push(&node);
			}
		}
	}

	// -- Calculate resources lifetime:

	for (auto& pass : mPassNodes)
	{
		if (pass.mRefCount == 0) continue;

		for (const auto id : pass.mCreates)
			_getResourceEntry(id).mProducer = &pass;
		for (const auto [id, _] : pass.mWrites)
			_getResourceEntry(id).mLast = &pass;
		for (const auto [id, _] : pass.mReads)
			_getResourceEntry(id).mLast = &pass;
	}
}

void FrameGraph::Execute(void* context, void* allocator)
{
	for (const auto& pass : mPassNodes)
	{
		if (!pass.canExecute()) continue;

		for (const auto id : pass.mCreates)
		{
			_getResourceEntry(id).create(allocator);
		}

		// 资源状态转换
		for (const auto [id, flags] : pass.mReads)
		{
			if (flags != kFlagsIgnored)
			{
				_getResourceEntry(id).preRead(flags, context);
			}
		}
		for (const auto [id, flags] : pass.mWrites)
		{
			if (flags != kFlagsIgnored)
			{
				_getResourceEntry(id).preWrite(flags, context);
			}
		}

		FrameGraphPassResources resources{ *this, pass };
		std::invoke(*pass.mExec, resources, context);

		for (auto& entry : mResourceRegistry)
		{
			if (entry.mLast == &pass && entry.isTransient())
			{
				entry.destroy(allocator);
			}
		}
	}
}

//
// (private):
//

PassNode& FrameGraph::_createPassNode(const std::string_view name, std::unique_ptr<FrameGraphPassConcept>&& base)
{
	const auto id = static_cast<uint32_t>(mPassNodes.size());
	return mPassNodes.emplace_back(PassNode{name, id, std::move(base)});
}

ResourceNode& FrameGraph::_createResourceNode(const std::string_view name, uint32_t resourceId, uint32_t version)
{
	const auto id = static_cast<uint32_t>(mResourceNodes.size());
	return mResourceNodes.emplace_back(ResourceNode{name, id, resourceId, version});
}

FrameGraphResource FrameGraph::_clone(FrameGraphResource id)
{
	const auto& node = _getResourceNode(id);
	auto& entry = _getResourceEntry(node);
	entry.mVersion++;

	const auto& clone = _createResourceNode(node.getName(), node.getResourceId(), entry.getVersion());
	return clone.getId();
}

const ResourceNode& FrameGraph::_getResourceNode(FrameGraphResource id) const
{
	assert(id < mResourceNodes.size());
	return mResourceNodes[id];
}

const ResourceEntry& FrameGraph::_getResourceEntry(FrameGraphResource id) const
{
	return _getResourceEntry(_getResourceNode(id));
}

const ResourceEntry& FrameGraph::_getResourceEntry(const ResourceNode& node) const
{
	assert(node.mResourceId < mResourceRegistry.size());
	return mResourceRegistry[node.mResourceId];
}

// 导出图表文件，本质是文本文件，方便检查，可视化，在线可视化工具如下：
// https://dreampuf.github.io/GraphvizOnline
RENDERSYSTEM_API std::ostream& operator<<(std::ostream& os, const FrameGraph& fg)
{
	return fg.DebugOutput(os, graphviz::Writer{});
}

//
// FrameGraph::Builder 实现部分
//

FrameGraphResource FrameGraph::Builder::Read(FrameGraphResource id, uint32_t flags)
{
	assert(mFrameGraph.IsValid(id));
	return mPassNode._read(id, flags);
}

FrameGraphResource FrameGraph::Builder::Write(FrameGraphResource id, uint32_t flags)
{
	assert(mFrameGraph.IsValid(id));
    if (mFrameGraph._getResourceEntry(id).isImported()) 
    {
        SetSideEffect();
    }

	if (mPassNode.creates(id))
	{
		return mPassNode._write(id, flags);
	}
	else
	{
		// Writing to a texture produces a renamed handle.
		// This allows us to catch errors when resources are modified in
		// undefined order (when same resource is written by different passes).
		// Renaming resources enforces a specific execution order of the render
		// passes.
		mPassNode._read(id, kFlagsIgnored);
		return mPassNode._write(mFrameGraph._clone(id), flags);
	}
}

NS_RENDERSYSTEM_END
