#include "FrameGraph/FrameGraphBuffer.h"
#include "FrameGraph/TransientResources.h"

NS_RENDERSYSTEM_BEGIN

void FrameGraphBuffer::create(const Desc& desc, void* allocator) 
{
	buffer = static_cast<TransientResources*>(allocator)->acquireBuffer(desc);
}

void FrameGraphBuffer::destroy(const Desc& desc, void* allocator) 
{
	static_cast<TransientResources*>(allocator)->releaseBuffer(desc, buffer);
}

std::string FrameGraphBuffer::toString(const Desc& desc) 
{
    char szBuf[1024] = {0};
    snprintf(szBuf, 1024, "size: %d bytes", desc.size);
    
    return std::string(szBuf);
}

NS_RENDERSYSTEM_END
