#pragma once

#include "Runtime/RenderCore/include/ComputeBuffer.h"

class FrameGraphBuffer
{
public:
	struct Desc
	{
		uint32_t size;
	};

	void create(const Desc&, void* allocator);
	void destroy(const Desc&, void* allocator);

	static std::string toString(const Desc&);

	RenderCore::ComputeBufferPtr buffer = nullptr;
};
