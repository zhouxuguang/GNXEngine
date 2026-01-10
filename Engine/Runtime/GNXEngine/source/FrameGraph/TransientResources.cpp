#include "FrameGraph/TransientResources.h"
#include "Runtime/BaseLib/include/HashFunction.h"


// desc的hash值
namespace std
{

template<> struct hash<GNXEngine::FrameGraphTexture::Desc>
{
    std::size_t operator()(const GNXEngine::FrameGraphTexture::Desc &desc) const noexcept 
	{
        std::size_t hash = baselib::HashFunction(&desc, sizeof(desc));
		return hash;
	}
};

template<> struct hash<GNXEngine::FrameGraphBuffer::Desc>
{
    std::size_t operator()(const GNXEngine::FrameGraphBuffer::Desc &desc) const noexcept
	{
        std::size_t hash = baselib::HashFunction(&desc, sizeof(desc));
        return hash;
	}
};

} // namespace std

NAMESPACE_GNXENGINE_BEGIN

namespace
{

template<typename T> using ResourcePool = std::vector<TransientResources::ResourceEntry<T>>;

template<typename T>
void HeartBeat(std::vector<T> &objects, std::unordered_map<std::size_t, ResourcePool<T> > &pools, float deltaTime, std::unordered_map<void*, std::string>& nameCache)
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
                    // 从名称缓存中移除
                    if (object)
                    {
                        nameCache.erase(object.get());
                    }
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
	// 清理空对象并从缓存中移除
	objects.erase(std::remove_if(objects.begin(), objects.end(),
		[&nameCache](auto &object) {
			if (!object)
			{
				nameCache.erase(object.get());
				return true;
			}
			return false;
		}), objects.end());
}

} // namespace

//
// TransientResources class:
//

TransientResources::TransientResources(RenderCore::RenderDevicePtr &renderDevice) : mRenderDevice(renderDevice) {}

TransientResources::~TransientResources()
{
	// 清理纹理
	for (auto &texture : m_textures)
    {
        if (texture)
        {
            m_resourceNameCache.erase(texture.get());
            texture.reset();
        }
    }

	// 清理 Buffer
	for (auto &buffer : m_buffers)
    {
        if (buffer)
        {
            m_resourceNameCache.erase(buffer.get());
            buffer.reset();
        }
    }

	// 清理池
	m_texturePools.clear();
	m_bufferPools.clear();

	m_textures.clear();
	m_buffers.clear();

	// 清理名称缓存
	m_resourceNameCache.clear();
}

void TransientResources::Update(float deltaTime)
{
    HeartBeat(m_textures, m_texturePools, deltaTime, m_resourceNameCache);
    HeartBeat(m_buffers, m_bufferPools, deltaTime, m_resourceNameCache);
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

        // 设置调试名称
        if (!desc.name.empty())
        {
            SetDebugName(texture, desc.name);
        }

        m_textures.push_back(texture);
        auto ptr = m_textures.back();
        return ptr;
    }
    else
    {
        auto texture = pool.back().resource;
        pool.pop_back();

        // 从资源池复用时也要更新名称
        if (!desc.name.empty())
        {
            SetDebugName(texture, desc.name);
        }

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

        // 设置调试名称
        if (!desc.name.empty())
        {
            SetDebugName(buffer, desc.name);
        }

        m_buffers.push_back(buffer);
        auto ptr = m_buffers.back();
        return ptr;
    }
    else
    {
        auto buffer = pool.back().resource;
        pool.pop_back();

        // 从资源池复用时也要更新名称
        if (!desc.name.empty())
        {
            SetDebugName(buffer, desc.name);
        }

        return buffer;
    }
}

void TransientResources::releaseBuffer(const FrameGraphBuffer::Desc &desc, RenderCore::ComputeBufferPtr buffer)
{
    const auto h = std::hash<FrameGraphBuffer::Desc>{}(desc);
    m_bufferPools[h].push_back({buffer, 0.0f});
}

void TransientResources::SetDebugName(RenderCore::RCTexturePtr texture, const std::string& name)
{
    if (texture && !name.empty())
    {
        void* ptr = texture.get();
        auto it = m_resourceNameCache.find(ptr);
        if (it == m_resourceNameCache.end() || it->second != name)
        {
            texture->SetName(name.c_str());
            m_resourceNameCache[ptr] = name;
        }
    }
}

void TransientResources::SetDebugName(RenderCore::ComputeBufferPtr buffer, const std::string& name)
{
    if (buffer && !name.empty())
    {
        void* ptr = buffer.get();
        auto it = m_resourceNameCache.find(ptr);
        if (it == m_resourceNameCache.end() || it->second != name)
        {
            buffer->SetName(name.c_str());
            m_resourceNameCache[ptr] = name;
        }
    }
}

NAMESPACE_GNXENGINE_END
