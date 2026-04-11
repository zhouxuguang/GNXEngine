#include "FrameGraph/TransientResources.h"
#include "Runtime/BaseLib/include/HashFunction.h"


// desc的hash值
namespace std
{

template<> struct hash<RenderSystem::FrameGraphTexture::Desc>
{
    std::size_t operator()(const RenderSystem::FrameGraphTexture::Desc &desc) const noexcept
	{
        std::size_t hash = baselib::HashFunction(&desc, sizeof(desc));
		return hash;
	}
};

template<> struct hash<RenderSystem::FrameGraphBuffer::Desc>
{
    std::size_t operator()(const RenderSystem::FrameGraphBuffer::Desc &desc) const noexcept
	{
        std::size_t hash = baselib::HashFunction(&desc, sizeof(desc));
        return hash;
	}
};

} // namespace std

NS_RENDERSYSTEM_BEGIN

namespace
{

template<typename T> using ResourcePool = std::vector<TransientResources::ResourceEntry<T>>;

template<typename T>
void HeartBeat(std::vector<T> &objects, std::unordered_map<std::size_t, ResourcePool<T> > &pools, float deltaTime)
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
	// 清理空对象并从缓存中移除
	objects.erase(std::remove_if(objects.begin(), objects.end(),
		[](auto& object) {
			if (!object)
			{
				return true;
			}
			return false;
		}), objects.end());
}

} // namespace

//
// TransientResources class:
//

TransientResources::TransientResources(RenderCore::RenderDevicePtr renderDevice) : mRenderDevice(renderDevice) {}

TransientResources::~TransientResources()
{
	// 清理纹理
	for (auto &texture : mTextures)
    {
        if (texture)
        {
            //m_resourceNameCache.erase(texture.get());
            texture.reset();
        }
    }

	// 清理 Buffer
	for (auto &buffer : mBuffers)
    {
        if (buffer)
        {
            //m_resourceNameCache.erase(buffer.get());
            buffer.reset();
        }
    }

	// 清理池
	mTexturePools.clear();
	mBufferPools.clear();

	mTextures.clear();
	mBuffers.clear();

	// 清理名称缓存
	//m_resourceNameCache.clear();
}

void TransientResources::Update(float deltaTime)
{
    HeartBeat(mTextures, mTexturePools, deltaTime);
    HeartBeat(mBuffers, mBufferPools, deltaTime);
}

RenderCore::RCTexturePtr TransientResources::acquireTexture(const FrameGraphTexture::Desc &desc)
{
    const auto h = std::hash<FrameGraphTexture::Desc>{}(desc);
    auto &pool = mTexturePools[h];
    if (pool.empty())
    {
        RenderCore::RCTexturePtr texture = nullptr;
        if (desc.depth > 0)
        {
            texture = mRenderDevice->CreateTexture2D(desc.format,
                                            RenderCore::TextureUsage::TextureUsageShaderRead | RenderCore::TextureUsage::TextureUsageRenderTarget,
                                            desc.extent.width, desc.extent.height, desc.numMipLevels);
        }
        else
        {
            texture = mRenderDevice->CreateTexture3D(desc.format,
                                            RenderCore::TextureUsage::TextureUsageShaderRead | RenderCore::TextureUsage::TextureUsageRenderTarget,
                                            desc.extent.width, desc.extent.height, desc.depth, desc.numMipLevels);
        }

        // 设置调试名称
        SetDebugName(texture, desc.name);

        mTextures.push_back(texture);
        auto ptr = mTextures.back();
        return ptr;
    }
    else
    {
        auto texture = pool.back().resource;
        pool.pop_back();

        // 检查从池中取出的资源是否有效
        if (!texture)
        {
            // 如果池中的资源无效，创建新资源
            texture = mRenderDevice->CreateTexture2D(desc.format,
                                            RenderCore::TextureUsage::TextureUsageShaderRead | RenderCore::TextureUsage::TextureUsageRenderTarget,
                                            desc.extent.width, desc.extent.height, desc.numMipLevels);
            mTextures.push_back(texture);
        }

        // 从资源池复用时也要更新名称
        SetDebugName(texture, desc.name);

        return texture;
    }
}

void TransientResources::releaseTexture(const FrameGraphTexture::Desc &desc, RenderCore::RCTexturePtr texture)
{
    const auto h = std::hash<FrameGraphTexture::Desc>{}(desc);
    mTexturePools[h].push_back({texture, 0.0f});
}

RenderCore::RCBufferPtr TransientResources::acquireBuffer(const FrameGraphBuffer::Desc &desc)
{
    const auto h = std::hash<FrameGraphBuffer::Desc>{}(desc);
    auto &pool = mBufferPools[h];
    if (pool.empty())
    {
        auto buffer = mRenderDevice->CreateBuffer(
            RenderCore::RCBufferDesc(desc.size, RenderCore::RCBufferUsage::StorageBuffer | 
                RenderCore::RCBufferUsage::VertexBuffer | RenderCore::RCBufferUsage::IndirectBuffer |
                RenderCore::RCBufferUsage::TransferSrc | RenderCore::RCBufferUsage::TransferDst));

        // 设置调试名称
        SetDebugName(buffer, desc.name);

        mBuffers.push_back(buffer);
        auto ptr = mBuffers.back();
        return ptr;
    }
    else
    {
        auto buffer = pool.back().resource;
        pool.pop_back();

        // 从资源池复用时也要更新名称
        SetDebugName(buffer, desc.name);

        return buffer;
    }
}

void TransientResources::releaseBuffer(const FrameGraphBuffer::Desc &desc, RenderCore::RCBufferPtr buffer)
{
    const auto h = std::hash<FrameGraphBuffer::Desc>{}(desc);
    mBufferPools[h].push_back({buffer, 0.0f});
}

void TransientResources::SetDebugName(RenderCore::RCTexturePtr texture, const std::string& name)
{
    texture->SetName(name.c_str());
}

void TransientResources::SetDebugName(RenderCore::RCBufferPtr buffer, const std::string& name)
{
    buffer->SetName(name.c_str());
}

NS_RENDERSYSTEM_END
