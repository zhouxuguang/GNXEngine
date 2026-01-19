#ifndef GNXENGINE_RENDERSYSYTEM_FRAMEGRAPH_FRAMEGRAPHTEXTURE_H
#define GNXENGINE_RENDERSYSYTEM_FRAMEGRAPH_FRAMEGRAPHTEXTURE_H

#include "Runtime/RenderCore/include/CommandBuffer.h"
#include "RSDefine.h"

NS_RENDERSYSTEM_BEGIN

class RENDERSYSTEM_API FrameGraphTexture
{
public:
	struct Desc
	{
		char name[64];
		RenderCore::Rect2D extent;
		uint32_t depth = 1;
		uint32_t numMipLevels = 1;
		uint32_t layers = 1;
		RenderCore::TextureFormat format = RenderCore::kTexFormatInvalid;

		//bool shadowSampler{ false };
		//WrapMode wrapMode{ WrapMode::ClampToEdge };
		//RenderCore::SamplerWrapMode filter = TexelFilter::Linear;
	};

	void create(const Desc& desc, void* allocator);
	void destroy(const Desc& desc, void* allocator);

	void preRead(const Desc& desc, uint32_t flags, void* context);
	void preWrite(const Desc& desc, uint32_t flags, void* context);

	static std::string toString(const Desc& desc);

	RenderCore::RCTexturePtr texture = nullptr;

private:
	RenderCore::ResourceAccessType DetermineAccessFlags(const Desc& desc, uint32_t flags, bool isWrite) const;
};

NS_RENDERSYSTEM_END
#endif
