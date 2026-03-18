#include "FrameGraph/FrameGraphTexture.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/TransientResources.h"
#include "FrameGraph/FrameGraphExecuteContext.h"
#include "Runtime/RenderCore/include/TextureFormat.h"

NS_RENDERSYSTEM_BEGIN

static std::string toString(RenderCore::Rect2D extent, uint32_t depth)
{
    char szBuf[128] = {0};
    if (depth > 1)
    {
        snprintf(szBuf, 128, "%dx%dx%d", extent.width, extent.height, depth);
    }
    else
    {
        snprintf(szBuf, 128, "%dx%d", extent.width, extent.height);
    }

    return std::string(szBuf);
}

void FrameGraphTexture::create(const Desc& desc, void* allocator)
{
	texture = static_cast<TransientResources*>(allocator)->acquireTexture(desc);
}

void FrameGraphTexture::destroy(const Desc& desc, void* allocator)
{
	static_cast<TransientResources*>(allocator)->releaseTexture(desc, texture);
}

void FrameGraphTexture::preRead(const Desc& desc, uint32_t flags, void* context)
{
	// Get FrameGraphExecuteContext from context
	if (!context || !texture)
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

	// Notify command buffer, RHI layer will automatically handle layout transition
	executeContext->commandBuffer->ResourceBarrier(texture, accessType);
}

void FrameGraphTexture::preWrite(const Desc& desc, uint32_t flags, void* context)
{
	// Get FrameGraphExecuteContext from context
	if (!context || !texture)
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

	// Notify command buffer, RHI layer will automatically handle layout transition
	executeContext->commandBuffer->ResourceBarrier(texture, accessType);
}

std::string FrameGraphTexture::toString(const Desc& desc)
{
    char szBuf[1024] = {0};
    snprintf(szBuf, 1024, "%s [%s]", RenderSystem::toString(desc.extent, desc.depth).c_str(), std::to_string(desc.format).c_str());

    return std::string(szBuf);
}

RenderCore::ResourceAccessType FrameGraphTexture::DetermineAccessFlags(const Desc& desc, uint32_t flags, bool isWrite) const
{
	RenderCore::ResourceAccessType accessFlags = static_cast<RenderCore::ResourceAccessType>(flags);

	if (flags != FrameGraph::kFlagsIgnored)
	{
		if ((accessFlags & RenderCore::ResourceAccessType::DepthStencilReadOnly) != static_cast<RenderCore::ResourceAccessType>(0))
		{
			return RenderCore::ResourceAccessType::DepthStencilReadOnly;
		}
		if ((accessFlags & RenderCore::ResourceAccessType::DepthStencilAttachment) != static_cast<RenderCore::ResourceAccessType>(0))
		{
			return RenderCore::ResourceAccessType::DepthStencilAttachment;
		}
		if ((accessFlags & RenderCore::ResourceAccessType::ComputeShaderWrite) != static_cast<RenderCore::ResourceAccessType>(0))
		{
			return RenderCore::ResourceAccessType::ComputeShaderWrite;
		}
		if ((accessFlags & RenderCore::ResourceAccessType::ColorAttachment) != static_cast<RenderCore::ResourceAccessType>(0))
		{
			return RenderCore::ResourceAccessType::ColorAttachment;
		}
		if ((accessFlags & RenderCore::ResourceAccessType::ComputeShaderRead) != static_cast<RenderCore::ResourceAccessType>(0))
		{
			return RenderCore::ResourceAccessType::ComputeShaderRead;
		}
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
	}

	if (isWrite)
	{
		// Write operation: determine if it's depth stencil attachment or color attachment based on texture format
		if (desc.format == RenderCore::kTexFormatDepth16 ||
			desc.format == RenderCore::kTexFormatDepth24Stencil8 ||
			desc.format == RenderCore::kTexFormatDepth32Float ||
			desc.format == RenderCore::kTexFormatDepth24 ||
			desc.format == RenderCore::kTexFormatDepth32FloatStencil8)
		{
			return RenderCore::ResourceAccessType::DepthStencilAttachment;
		}

		return RenderCore::ResourceAccessType::ColorAttachment;
	}

	// Read operation: default to shader read
	return RenderCore::ResourceAccessType::ShaderRead;
}

NS_RENDERSYSTEM_END
