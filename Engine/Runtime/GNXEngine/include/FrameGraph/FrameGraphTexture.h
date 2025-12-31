#pragma once

#include "Runtime/RenderCore/include/RCTexture.h"

class FrameGraphTexture
{
public:
	struct Desc 
	{
		RenderCore::Rect2D extent;
		uint32_t depth = 1;
		uint32_t numMipLevels = 1;
		uint32_t layers = 0;
		RenderCore::TextureFormat format = RenderCore::kTexFormatInvalid;

		bool shadowSampler{ false };
		//WrapMode wrapMode{ WrapMode::ClampToEdge };
		//RenderCore::SamplerWrapMode filter = TexelFilter::Linear;
	};

	void create(const Desc& desc, void* allocator);
	void destroy(const Desc& desc, void* allocator);

	static std::string toString(const Desc& desc);

	RenderCore::RCTexturePtr texture = nullptr;
};
