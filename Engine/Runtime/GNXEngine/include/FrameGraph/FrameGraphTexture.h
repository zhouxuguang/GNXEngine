#pragma once

#include "Runtime/RenderCore/include/RCTexture.h"

class FrameGraphTexture
{
public:
	struct Desc 
	{
		RenderCore::Rect2D extent;
		uint32_t depth = 0;
		uint32_t numMipLevels = 1;
		uint32_t layers = 0;
		RenderCore::TextureFormat format = RenderCore::kTexFormatInvalid;

		bool shadowSampler{ false };
		//WrapMode wrapMode{ WrapMode::ClampToEdge };
		//RenderCore::SamplerWrapMode filter = TexelFilter::Linear;
	};

	void create(const Desc&, void* allocator);
	void destroy(const Desc&, void* allocator);

	static std::string toString(const Desc&);

	RenderCore::RCTexturePtr texture = nullptr;
};
