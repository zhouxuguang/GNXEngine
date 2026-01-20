#include "FrameGraph/FrameGraphBuffer.h"
#include "FrameGraph/TransientResources.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/FrameGraphExecuteContext.h"
#include "Runtime/RenderCore/include/CommandBuffer.h"

NS_RENDERSYSTEM_BEGIN

void FrameGraphBuffer::create(const Desc& desc, void* allocator)
{
	buffer = static_cast<TransientResources*>(allocator)->acquireBuffer(desc);
}

void FrameGraphBuffer::destroy(const Desc& desc, void* allocator)
{
	static_cast<TransientResources*>(allocator)->releaseBuffer(desc, buffer);
}

void FrameGraphBuffer::preRead(const Desc& desc, uint32_t flags, void* context)
{
	// Get FrameGraphExecuteContext from context
	if (!context || !buffer)
	{
		return;
	}

	const FrameGraphExecuteContext* executeContext = static_cast<const FrameGraphExecuteContext*>(context);
	if (!executeContext || !executeContext->commandBuffer)
	{
		return;
	}

	// Determine access type
	RenderCore::ResourceAccessType accessType = DetermineAccessFlags(desc, flags, false);

	// Notify command buffer, RHI layer will automatically handle barrier
	executeContext->commandBuffer->ResourceBarrier(buffer, accessType);
}

void FrameGraphBuffer::preWrite(const Desc& desc, uint32_t flags, void* context)
{
	// Get FrameGraphExecuteContext from context
	if (!context || !buffer)
	{
		return;
	}

	const FrameGraphExecuteContext* executeContext = static_cast<const FrameGraphExecuteContext*>(context);
	if (!executeContext || !executeContext->commandBuffer)
	{
		return;
	}

	// Determine access type
	RenderCore::ResourceAccessType accessType = DetermineAccessFlags(desc, flags, true);

	// Notify command buffer, RHI layer will automatically handle barrier
	executeContext->commandBuffer->ResourceBarrier(buffer, accessType);
}

std::string FrameGraphBuffer::toString(const Desc& desc)
{
    char szBuf[1024] = {0};
    snprintf(szBuf, 1024, "size: %d bytes", desc.size);

    return std::string(szBuf);
}

RenderCore::ResourceAccessType FrameGraphBuffer::DetermineAccessFlags(const Desc& desc, uint32_t flags, bool isWrite) const
{
	RenderCore::ResourceAccessType accessFlags = static_cast<RenderCore::ResourceAccessType>(flags);

	if (flags != FrameGraph::kFlagsIgnored)
	{
		// Check for specific access flags
		if ((accessFlags & RenderCore::ResourceAccessType::ShaderRead) != static_cast<RenderCore::ResourceAccessType>(0))
		{
			return RenderCore::ResourceAccessType::ShaderRead;
		}
		if ((accessFlags & RenderCore::ResourceAccessType::TransferSrc) != static_cast<RenderCore::ResourceAccessType>(0))
		{
			return RenderCore::ResourceAccessType::TransferSrc;
		}
		if ((accessFlags & RenderCore::ResourceAccessType::TransferDst) != static_cast<RenderCore::ResourceAccessType>(0))
		{
			return RenderCore::ResourceAccessType::TransferDst;
		}
		if ((accessFlags & RenderCore::ResourceAccessType::ComputeShaderRead) != static_cast<RenderCore::ResourceAccessType>(0))
		{
			return RenderCore::ResourceAccessType::ComputeShaderRead;
		}
		if ((accessFlags & RenderCore::ResourceAccessType::ComputeShaderWrite) != static_cast<RenderCore::ResourceAccessType>(0))
		{
			return RenderCore::ResourceAccessType::ComputeShaderWrite;
		}
	}

	if (isWrite)
	{
		// Write operation: default to Write for buffers
		return RenderCore::ResourceAccessType::ComputeShaderWrite;
	}

	// Read operation: default to compute shader read
	return RenderCore::ResourceAccessType::ComputeShaderRead;
}

NS_RENDERSYSTEM_END
