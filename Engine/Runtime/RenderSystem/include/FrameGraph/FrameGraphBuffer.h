#ifndef GNXENGINE_RENDERSYSYTEM_FRAMEGRAPH_FRAMEGRAPHBUFFER_H
#define GNXENGINE_RENDERSYSYTEM_FRAMEGRAPH_FRAMEGRAPHBUFFER_H

#include "Runtime/RenderCore/include/RCBuffer.h"
#include "../RSDefine.h"

NS_RENDERSYSTEM_BEGIN

class RENDERSYSTEM_API FrameGraphBuffer
{
public:
	struct Desc
	{
		char name[64] = {};
		uint32_t size = 0;

		// 便捷的名称设置方法
		void SetName(const char* str)
		{
			snprintf(name, sizeof(name), "%s", str ? str : "");
		}
		
		void SetName(const std::string& str)
		{
			snprintf(name, sizeof(name), "%s", str.c_str());
		}
	};

	void create(const Desc& desc, void* allocator);
	void destroy(const Desc& desc, void* allocator);

	void preRead(const Desc& desc, uint32_t flags, void* context);
	void preWrite(const Desc& desc, uint32_t flags, void* context);

	static std::string toString(const Desc& desc);

	RenderCore::RCBufferPtr buffer = nullptr;

private:
	RenderCore::ResourceAccessType DetermineAccessFlags(const Desc& desc, uint32_t flags, bool isWrite) const;
};

NS_RENDERSYSTEM_END

#endif
