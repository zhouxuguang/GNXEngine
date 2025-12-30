#include "FrameGraph/FrameGraphTexture.h"
//#include "TransientResources.h"
#include <format>

static std::string toString(RenderCore::Rect2D extent, uint32_t depth) 
{
	return depth > 1 ? std::format("{}x{}x{}", extent.width, extent.height, depth)
		: std::format("{}x{}", extent.width, extent.height);
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
	return std::format("{} [{}]", ::toString(desc.extent, desc.depth), std::to_string(desc.format));
}
