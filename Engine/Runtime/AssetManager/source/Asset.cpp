#include "Asset.h"
#include <cassert>

NS_ASSETMANAGER_BEGIN

Asset::Asset()
	: m_refCount(0)
	, m_state(AssetState::Unloaded)
	, m_fileSize(0)
	, m_memorySize(0)
	, m_lastModified(0)
{
}

Asset::~Asset()
{
	// 确保资源已被正确卸载
	assert(m_refCount == 0 && "Asset destroyed with non-zero reference count!");
}

void Asset::AddRef()
{
	++m_refCount;
}

void Asset::Release()
{
	assert(m_refCount > 0 && "Asset reference count underflow!");
	--m_refCount;
}

int Asset::GetRefCount() const
{
	return m_refCount.load();
}

AssetState Asset::GetState() const
{
	return m_state;
}

void Asset::SetState(AssetState state)
{
	m_state = state;
}

uint64_t Asset::GetFileSize() const
{
	return m_fileSize;
}

void Asset::SetFileSize(uint64_t size)
{
	m_fileSize = size;
}

uint64_t Asset::GetMemorySize() const
{
	return m_memorySize;
}

void Asset::SetMemorySize(uint64_t size)
{
	m_memorySize = size;
}

int64_t Asset::GetLastModified() const
{
	return m_lastModified;
}

void Asset::SetLastModified(int64_t timestamp)
{
	m_lastModified = timestamp;
}

NS_ASSETMANAGER_END
