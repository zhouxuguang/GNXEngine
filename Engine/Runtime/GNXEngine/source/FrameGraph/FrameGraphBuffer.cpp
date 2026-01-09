#include "FrameGraph/FrameGraphBuffer.h"
#include "FrameGraph/TransientResources.h"

NAMESPACE_GNXENGINE_BEGIN

// 根据 flags 获取 RHI 访问类型
static RenderCore::ResourceAccess GetResourceAccess(uint32_t flags)
{
    return static_cast<RenderCore::ResourceAccess>(flags);
}

// 根据 ResourceAccess 获取管线阶段
static RenderCore::ResourcePipelineStage GetPipelineStage(RenderCore::ResourceAccess access)
{
    if ((access & (RenderCore::ResourceAccess::VertexBuffer | RenderCore::ResourceAccess::IndexBuffer)) != RenderCore::ResourceAccess::None)
    {
        return RenderCore::ResourcePipelineStage::VertexInput;
    }
    if ((access & RenderCore::ResourceAccess::UniformBuffer) != RenderCore::ResourceAccess::None)
    {
        return RenderCore::ResourcePipelineStage::VertexShader;
    }
    if ((access & (RenderCore::ResourceAccess::StorageBufferRead | RenderCore::ResourceAccess::StorageBufferWrite)) != RenderCore::ResourceAccess::None)
    {
        return RenderCore::ResourcePipelineStage::ComputeShader;
    }
    if ((access & RenderCore::ResourceAccess::ColorAttachment) != RenderCore::ResourceAccess::None)
    {
        return RenderCore::ResourcePipelineStage::ColorAttachmentOutput;
    }
    if ((access & RenderCore::ResourceAccess::DepthStencilAttachment) != RenderCore::ResourceAccess::None)
    {
        return RenderCore::ResourcePipelineStage::LateFragmentTests;
    }
    if ((access & RenderCore::ResourceAccess::ShaderResource) != RenderCore::ResourceAccess::None)
    {
        return RenderCore::ResourcePipelineStage::FragmentShader;
    }
    if ((access & RenderCore::ResourceAccess::ComputeShaderResource) != RenderCore::ResourceAccess::None)
    {
        return RenderCore::ResourcePipelineStage::ComputeShader;
    }
    if ((access & (RenderCore::ResourceAccess::TransferSrc | RenderCore::ResourceAccess::TransferDst)) != RenderCore::ResourceAccess::None)
    {
        return RenderCore::ResourcePipelineStage::Transfer;
    }
    return RenderCore::ResourcePipelineStage::BottomOfPipe;
}

// 判断是否需要屏障
static bool NeedsBarrier(const RenderCore::ResourceState& current, 
                         RenderCore::ResourceAccess newAccess, 
                         RenderCore::ResourcePipelineStage newStage)
{
    if (!current.initialized)
    {
        return false;
    }
    
    if (current.access != newAccess || current.stage != newStage)
    {
        return true;
    }
    
    return false;
}

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
	if (!buffer || !context)
	{
		return;
	}
	
	// 将 flags 转换为 RHI 访问类型
	RenderCore::ResourceAccess accessType = GetResourceAccess(flags);
	RenderCore::ResourcePipelineStage pipelineStage = GetPipelineStage(accessType);
	
	// 获取缓冲区的当前状态
	RenderCore::ResourceState currentState = buffer->GetState();
	
	// 检查是否需要屏障
	if (NeedsBarrier(currentState, accessType, pipelineStage))
	{
		// 调用 RHI 抽象接口插入屏障
		buffer->PreReadBarrier(context, accessType, pipelineStage);
		
		// 更新缓冲区状态
		RenderCore::ResourceState newState;
		newState.access = accessType;
		newState.stage = pipelineStage;
		newState.initialized = true;
		buffer->SetState(newState);
	}
}

void FrameGraphBuffer::preWrite(const Desc& desc, uint32_t flags, void* context)
{
	if (!buffer || !context)
	{
		return;
	}
	
	// 将 flags 转换为 RHI 访问类型
	RenderCore::ResourceAccess accessType = GetResourceAccess(flags);
	RenderCore::ResourcePipelineStage pipelineStage = GetPipelineStage(accessType);
	
	// 获取缓冲区的当前状态
	RenderCore::ResourceState currentState = buffer->GetState();
	
	// 检查是否需要屏障
	if (NeedsBarrier(currentState, accessType, pipelineStage))
	{
		// 调用 RHI 抽象接口插入屏障
		buffer->PreWriteBarrier(context, accessType, pipelineStage);
		
		// 更新缓冲区状态
		RenderCore::ResourceState newState;
		newState.access = accessType;
		newState.stage = pipelineStage;
		newState.initialized = true;
		buffer->SetState(newState);
	}
}

std::string FrameGraphBuffer::toString(const Desc& desc) 
{
    char szBuf[1024] = {0};
    snprintf(szBuf, 1024, "size: %d bytes", desc.size);
    
    return std::string(szBuf);
}

NAMESPACE_GNXENGINE_END
