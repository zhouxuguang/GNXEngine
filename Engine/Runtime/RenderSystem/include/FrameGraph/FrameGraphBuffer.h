#ifndef GNXENGINE_RENDERSYSYTEM_FRAMEGRAPH_FRAMEGRAPHBUFFER_H
#define GNXENGINE_RENDERSYSYTEM_FRAMEGRAPH_FRAMEGRAPHBUFFER_H

#include "Runtime/RenderCore/include/ComputeBuffer.h"
#include "../RSDefine.h"

NS_RENDERSYSTEM_BEGIN

class RENDERSYSTEM_API FrameGraphBuffer
{
public:
	struct Desc
	{
		char name[64];
		uint32_t size;
	};

	void create(const Desc& desc, void* allocator);
	void destroy(const Desc& desc, void* allocator);

	static std::string toString(const Desc& desc);

	RenderCore::ComputeBufferPtr buffer = nullptr;
};

NS_RENDERSYSTEM_END

#endif
