#pragma once

#include "Runtime/RenderCore/include/ComputeBuffer.h"
#include "PreDefine.h"

NAMESPACE_GNXENGINE_BEGIN

class GNXENGINE_API FrameGraphBuffer
{
public:
	struct Desc
	{
		uint32_t size;
	};

	void create(const Desc& desc, void* allocator);
	void destroy(const Desc& desc, void* allocator);

	static std::string toString(const Desc& desc);

	RenderCore::ComputeBufferPtr buffer = nullptr;
};

NAMESPACE_GNXENGINE_END
