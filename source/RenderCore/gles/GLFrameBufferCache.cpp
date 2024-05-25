//
//  GLFrameBufferCache.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/30.
//

#include "GLFrameBufferCache.h"

NAMESPACE_RENDERCORE_BEGIN

GLFrameBufferPtr GLFrameBufferCache::GetCacheFBO(const FboCacheItem& cacheItem) const
{
    auto iterFind = mCachedFBOs.find(cacheItem);
    if (iterFind != mCachedFBOs.end())
    {
        return iterFind->second;
    }
    return nullptr;
}

void GLFrameBufferCache::AddFBO(const FboCacheItem& cacheItem, GLFrameBufferPtr fboID)
{
    mCachedFBOs.insert(std::make_pair(cacheItem, fboID));
}

GLFrameBufferCache *GLFrameBufferCache::getInstance()
{
    static GLFrameBufferCache instance;
    return &instance;
}

NAMESPACE_RENDERCORE_END
