#pragma once

#include "FrameGraphTexture.h"
#include "FrameGraphBuffer.h"
#include "PreDefine.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include <memory>
#include <vector>
#include <unordered_map>

// 瞬间时间资源管理，配合帧图一起使用
class GNXENGINE_API TransientResources
{
public:
	TransientResources() = delete;
	explicit TransientResources(RenderCore::RenderDevicePtr &renderDevice);
	TransientResources(const TransientResources &) = delete;
	TransientResources(TransientResources &&) noexcept = delete;
	~TransientResources();

	TransientResources &operator=(const TransientResources &) = delete;
	TransientResources &operator=(TransientResources &&) noexcept = delete;

	void update(float dt);

	[[nodiscard]] RenderCore::RCTexturePtr acquireTexture(const FrameGraphTexture::Desc &);
	void releaseTexture(const FrameGraphTexture::Desc &, RenderCore::RCTexturePtr);

	[[nodiscard]] RenderCore::ComputeBufferPtr acquireBuffer(const FrameGraphBuffer::Desc &);
	void releaseBuffer(const FrameGraphBuffer::Desc &, RenderCore::ComputeBufferPtr);
    
    template<typename T> struct ResourceEntry
    {
        T resource;
        float life;
    };

private:
    RenderCore::RenderDevicePtr &mRenderDevice;

	std::vector<RenderCore::RCTexturePtr> m_textures;
	std::vector<RenderCore::ComputeBufferPtr> m_buffers;
    
    template<typename T> using ResourcePool = std::vector<ResourceEntry<T>>;

	std::unordered_map<std::size_t, ResourcePool<RenderCore::RCTexturePtr>> m_texturePools;
	std::unordered_map<std::size_t, ResourcePool<RenderCore::ComputeBufferPtr>> m_bufferPools;
};
