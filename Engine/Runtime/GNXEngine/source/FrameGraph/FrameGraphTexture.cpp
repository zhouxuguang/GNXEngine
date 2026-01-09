#include "FrameGraph/FrameGraphTexture.h"
#include "FrameGraph/TransientResources.h"

NAMESPACE_GNXENGINE_BEGIN

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

// 根据 ResourceAccess 获取图像布局
static RenderCore::ResourceLayout GetImageLayout(RenderCore::ResourceAccess access)
{
    if ((access & RenderCore::ResourceAccess::ColorAttachment) != RenderCore::ResourceAccess::None)
    {
        return RenderCore::ResourceLayout::ColorAttachmentOptimal;
    }
    if ((access & RenderCore::ResourceAccess::DepthStencilAttachment) != RenderCore::ResourceAccess::None)
    {
        return RenderCore::ResourceLayout::DepthStencilAttachmentOptimal;
    }
    if ((access & RenderCore::ResourceAccess::DepthStencilReadOnly) != RenderCore::ResourceAccess::None)
    {
        return RenderCore::ResourceLayout::DepthStencilReadOnlyOptimal;
    }
    if ((access & (RenderCore::ResourceAccess::ShaderResource | RenderCore::ResourceAccess::ComputeShaderResource)) != RenderCore::ResourceAccess::None)
    {
        return RenderCore::ResourceLayout::ShaderReadOnlyOptimal;
    }
    if ((access & RenderCore::ResourceAccess::TransferSrc) != RenderCore::ResourceAccess::None)
    {
        return RenderCore::ResourceLayout::TransferSrcOptimal;
    }
    if ((access & RenderCore::ResourceAccess::TransferDst) != RenderCore::ResourceAccess::None)
    {
        return RenderCore::ResourceLayout::TransferDstOptimal;
    }
    return RenderCore::ResourceLayout::General;
}

// 判断是否需要屏障
static bool NeedsBarrier(const RenderCore::ResourceState& current, 
                         RenderCore::ResourceAccess newAccess, 
                         RenderCore::ResourcePipelineStage newStage, 
                         RenderCore::ResourceLayout newLayout)
{
    if (!current.initialized)
    {
        return false;
    }
    
    if (current.access != newAccess || 
        current.stage != newStage || 
        current.layout != newLayout)
    {
        return true;
    }
    
    return false;
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
	if (!texture || !context)
	{
		return;
	}
	
	// 将 flags 转换为 RHI 访问类型
	RenderCore::ResourceAccess accessType = GetResourceAccess(flags);
	RenderCore::ResourcePipelineStage pipelineStage = GetPipelineStage(accessType);
	RenderCore::ResourceLayout imageLayout = GetImageLayout(accessType);
	
	// 获取纹理的当前状态
	RenderCore::ResourceState currentState = texture->GetState();
	
	// 检查是否需要屏障
	if (NeedsBarrier(currentState, accessType, pipelineStage, imageLayout))
	{
		// 调用 RHI 抽象接口插入屏障
		texture->PreReadBarrier(context, accessType, pipelineStage, imageLayout);
		
		// 更新纹理状态
		RenderCore::ResourceState newState;
		newState.access = accessType;
		newState.stage = pipelineStage;
		newState.layout = imageLayout;
		newState.initialized = true;
		texture->SetState(newState);
	}
}

void FrameGraphTexture::preWrite(const Desc& desc, uint32_t flags, void* context)
{
	if (!texture || !context)
	{
		return;
	}
	
	// 将 flags 转换为 RHI 访问类型
	RenderCore::ResourceAccess accessType = GetResourceAccess(flags);
	RenderCore::ResourcePipelineStage pipelineStage = GetPipelineStage(accessType);
	RenderCore::ResourceLayout imageLayout = GetImageLayout(accessType);
	
	// 获取纹理的当前状态
	RenderCore::ResourceState currentState = texture->GetState();
	
	// 检查是否需要屏障
	if (NeedsBarrier(currentState, accessType, pipelineStage, imageLayout))
	{
		// 调用 RHI 抽象接口插入屏障
		texture->PreWriteBarrier(context, accessType, pipelineStage, imageLayout);
		
		// 更新纹理状态
		RenderCore::ResourceState newState;
		newState.access = accessType;
		newState.stage = pipelineStage;
		newState.layout = imageLayout;
		newState.initialized = true;
		texture->SetState(newState);
	}
}

std::string FrameGraphTexture::toString(const Desc& desc) 
{
    char szBuf[1024] = {0};
    snprintf(szBuf, 1024, "%s [%s]", GNXEngine::toString(desc.extent, desc.depth).c_str(), std::to_string(desc.format).c_str());
    
    return std::string(szBuf);
}

NAMESPACE_GNXENGINE_END
