#pragma once

#include "Runtime/RenderCore/include/RCTexture.h"
#include "PreDefine.h"

NAMESPACE_GNXENGINE_BEGIN

class GNXENGINE_API FrameGraphTexture
{
public:
	struct Desc
	{
		std::string name;
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

	static std::string toString(const Desc& desc);

	RenderCore::RCTexturePtr texture = nullptr;
};

NAMESPACE_GNXENGINE_END
