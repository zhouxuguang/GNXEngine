#include "FrameGraph/TransientResources.h"
#include "Runtime/BaseLib/include/HashFunction.h"


// desc的hash值
namespace std
{

template<> struct hash<FrameGraphTexture::Desc>
{
	std::size_t operator()(const FrameGraphTexture::Desc &desc) const noexcept 
	{
        std::size_t hash = baselib::HashFunction(&desc, sizeof(desc));
		return hash;
	}
};

template<> struct hash<FrameGraphBuffer::Desc>
{
	std::size_t operator()(const FrameGraphBuffer::Desc &desc) const noexcept
	{
        std::size_t hash = baselib::HashFunction(&desc, sizeof(desc));
        return hash;
	}
};

} // namespace std

namespace 
{

template<typename T> using ResourcePool = std::vector<TransientResources::ResourceEntry<T>>;

template<typename T>
void HeartBeat(std::vector<T> &objects, std::unordered_map<std::size_t, ResourcePool<T>> &pools, float deltaTime)
{
	constexpr auto kMaxIdleTime = 1.0f; // in seconds

	auto poolIt = pools.begin();
	while (poolIt != pools.end()) 
	{
		auto &[_, pool] = *poolIt;
		if (pool.empty()) 
		{
			poolIt = pools.erase(poolIt);
		} 
		else 
		{
			auto objectIt = pool.begin();
			while (objectIt != pool.cend()) 
            {
                auto &[object, idleTime] = *objectIt;
                idleTime += deltaTime;
                if (idleTime >= kMaxIdleTime)
                {
                    object = nullptr;
                    objectIt = pool.erase(objectIt);
                } 
                else
                {
                    ++objectIt;
                }
			}
			++poolIt;
		}
	}
	objects.erase(std::remove_if(objects.begin(), objects.end(),
								[](auto &object) { return !object; }), objects.end());
}

} // namespace

//
// TransientResources class:
//

TransientResources::TransientResources(RenderCore::RenderDevicePtr &renderDevice) : mRenderDevice(renderDevice) {}

TransientResources::~TransientResources() 
{
	for (auto &texture : m_textures)
    {
        //
    }
	for (auto &buffer : m_buffers)
    {
        //
    }
}

void TransientResources::update(float dt) 
{
    HeartBeat(m_textures, m_texturePools, dt);
    HeartBeat(m_buffers, m_bufferPools, dt);
}

RenderCore::RCTexturePtr TransientResources::acquireTexture(const FrameGraphTexture::Desc &desc)
{
    const auto h = std::hash<FrameGraphTexture::Desc>{}(desc);
    auto &pool = m_texturePools[h];
    if (pool.empty()) 
    {
        RenderCore::RCTexturePtr texture = nullptr;
        if (desc.depth > 0)
        {
            texture = mRenderDevice->CreateTexture2D(desc.format,
                                            RenderCore::TextureUsage(RenderCore::TextureUsageShaderRead | RenderCore::TextureUsageRenderTarget),
                                            desc.extent.width, desc.extent.height, desc.numMipLevels);
        }
        else
        {
            texture = mRenderDevice->CreateTexture3D(desc.format,
                                            RenderCore::TextureUsage(RenderCore::TextureUsageShaderRead | RenderCore::TextureUsageRenderTarget),
                                            desc.extent.width, desc.extent.height, desc.depth, desc.numMipLevels);
        }

        m_textures.push_back(texture);
        auto ptr = m_textures.back();
        return ptr;
    } 
    else
    {
        auto texture = pool.back().resource;
        pool.pop_back();
        return texture;
    }
}

void TransientResources::releaseTexture(const FrameGraphTexture::Desc &desc, RenderCore::RCTexturePtr texture)
{
    const auto h = std::hash<FrameGraphTexture::Desc>{}(desc);
    m_texturePools[h].push_back({texture, 0.0f});
}

RenderCore::ComputeBufferPtr TransientResources::acquireBuffer(const FrameGraphBuffer::Desc &desc)
{
    const auto h = std::hash<FrameGraphBuffer::Desc>{}(desc);
    auto &pool = m_bufferPools[h];
    if (pool.empty())
    {
        auto buffer = mRenderDevice->CreateComputeBuffer(desc.size);
        m_buffers.push_back(buffer);
        auto ptr = m_buffers.back();
        return ptr;
    }
    else
    {
        auto buffer = pool.back().resource;
        pool.pop_back();
        return buffer;
    }
}

void TransientResources::releaseBuffer(const FrameGraphBuffer::Desc &desc, RenderCore::ComputeBufferPtr buffer)
{
    const auto h = std::hash<FrameGraphBuffer::Desc>{}(desc);
    m_bufferPools[h].push_back({buffer, 0.0f});
}
