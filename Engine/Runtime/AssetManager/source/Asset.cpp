#include "Asset.h"
#include <cassert>

NS_ASSETMANAGER_BEGIN

Asset::Asset()
	: mRefCount(0)
	, mState(AssetState::Unloaded)
	, mFileSize(0)
	, mMemorySize(0)
	, mLastModified(0)
{
}

Asset::~Asset()
{
	// 确保资源已被正确卸载
	assert(mRefCount == 0 && "Asset destroyed with non-zero reference count!");
}

void Asset::AddRef()
{
	++mRefCount;
}

void Asset::Release()
{
	assert(mRefCount > 0 && "Asset reference count underflow!");
	--mRefCount;
}

int Asset::GetRefCount() const
{
	return mRefCount.load();
}

AssetState Asset::GetState() const
{
	return mState;
}

void Asset::SetState(AssetState state)
{
	mState = state;
}

uint64_t Asset::GetFileSize() const
{
	return mFileSize;
}

void Asset::SetFileSize(uint64_t size)
{
	mFileSize = size;
}

uint64_t Asset::GetMemorySize() const
{
	return mMemorySize;
}

void Asset::SetMemorySize(uint64_t size)
{
	mMemorySize = size;
}

int64_t Asset::GetLastModified() const
{
	return mLastModified;
}

void Asset::SetLastModified(int64_t timestamp)
{
	mLastModified = timestamp;
}

NS_ASSETMANAGER_END
