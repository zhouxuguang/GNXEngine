#pragma once

#include "Runtime/RenderCore/include/RCTexture.h"

class FrameGraphTexture
{
public:
    FrameGraphTexture() = default;
    FrameGraphTexture(FrameGraphTexture&&) = default;
    FrameGraphTexture& operator=(FrameGraphTexture&&) = default;

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

	void create(const Desc&, void* allocator);
	void destroy(const Desc&, void* allocator);

	static std::string toString(const Desc&);

	RenderCore::RCTexturePtr texture = nullptr;
};
