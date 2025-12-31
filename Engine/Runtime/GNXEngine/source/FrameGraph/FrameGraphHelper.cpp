#include "FrameGraph/FrameGraphHelper.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/FrameGraphTexture.h"
#include "FrameGraph/FrameGraphBuffer.h"
#include <assert.h>

FrameGraphResource importTexture(FrameGraph& fg, const std::string_view name, RenderCore::RCTexturePtr texture)
{
	assert(texture);

    FrameGraphTexture::Desc desc = {};
    desc.depth = texture->GetDepth();
    desc.numMipLevels = 1; // todo 增加mipmap level
    // FORMAT
    desc.extent.offsetX = 0;
    desc.extent.offsetY = 0;
    desc.extent.width = texture->GetWidth();
    desc.extent.height = texture->GetHeight();

    FrameGraphTexture fgTexture;
    fgTexture.texture = texture;
    return fg.Import<FrameGraphTexture>(name, desc, std::move(fgTexture));
}

RenderCore::RCTexturePtr getTexture(FrameGraphPassResources& resources, FrameGraphResource id)
{
	return resources.Get<FrameGraphTexture>(id).texture;
}

FrameGraphResource importBuffer(FrameGraph& fg, const std::string_view name, RenderCore::ComputeBufferPtr buffer)
{
	assert(buffer);
    
    FrameGraphBuffer::Desc desc = {};
    desc.size = buffer->GetBufferLength();
    
    FrameGraphBuffer fgBuffer;
    fgBuffer.buffer = buffer;
    return fg.Import<FrameGraphBuffer>(name, desc, std::move(fgBuffer));
}

RenderCore::ComputeBufferPtr getBuffer(FrameGraphPassResources& resources, FrameGraphResource id)
{
	return resources.Get<FrameGraphBuffer>(id).buffer;
}

