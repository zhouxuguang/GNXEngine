#include "FrameGraph/FrameGraphTexture.h"
//#include "TransientResources.h"

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
	//texture = static_cast<TransientResources*>(allocator)->acquireTexture(desc);
}

void FrameGraphTexture::destroy(const Desc& desc, void* allocator) 
{
	//static_cast<TransientResources*>(allocator)->releaseTexture(desc, texture);
}

std::string FrameGraphTexture::toString(const Desc& desc) 
{
    char szBuf[1024] = {0};
    snprintf(szBuf, 1024, "%s [%s]", ::toString(desc.extent, desc.depth).c_str(), std::to_string(desc.format).c_str());
    
    return std::string(szBuf);
}
