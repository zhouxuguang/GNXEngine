#include "FrameGraph/FrameGraphTexture.h"
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
	// 从context获取FrameGraphExecuteContext
	if (!context || !texture)
	{
		return;
	}

	const FrameGraphExecuteContext* executeContext = static_cast<const FrameGraphExecuteContext*>(context);
	if (!executeContext || !executeContext->commandBuffer)
	{
		return;
	}

	// 确定访问类型
	RenderCore::ResourceAccessType accessType = DetermineAccessFlags(desc, flags, false);

	// 通知命令缓冲区，RHI层会自动处理layout转换
	executeContext->commandBuffer->ResourceBarrier(texture, accessType);
}

void FrameGraphTexture::preWrite(const Desc& desc, uint32_t flags, void* context)
{
	// 从context获取FrameGraphExecuteContext
	if (!context || !texture)
	{
		return;
	}

	const FrameGraphExecuteContext* executeContext = static_cast<const FrameGraphExecuteContext*>(context);
	if (!executeContext || !executeContext->commandBuffer)
	{
		return;
	}

	// 确定访问类型
	RenderCore::ResourceAccessType accessType = DetermineAccessFlags(desc, flags, true);

	// 通知命令缓冲区，RHI层会自动处理layout转换
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
	// 根据flags和访问类型确定ResourceAccessType
	// 这是RHI无关的抽象，由各RHI后端自行解释

	if (isWrite)
	{
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
	else
	{
		// 读操作：默认为着色器读取
		return RenderCore::ResourceAccessType::ShaderRead;
	}
}

NS_RENDERSYSTEM_END
