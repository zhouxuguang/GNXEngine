//
//  GLFrameBufferCache.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/30.
//

#ifndef GNX_ENGINE_GL_FRAMEBUFFER_CACHE_INCKUDE_DGV
#define GNX_ENGINE_GL_FRAMEBUFFER_CACHE_INCKUDE_DGV

#include "GLRenderDefine.h"
#include "GLFrameBuffer.h"
#include <unordered_map>

struct FboCacheItem
{
    uint32_t width;
    uint32_t height;
    
    bool operator==(const FboCacheItem &cacheItem) const
    {
        if (this->width != cacheItem.width || this->height != cacheItem.height)
        {
            return false;
        }
        return true;
    }
};

//自定义哈希函数
namespace std
{
    template <> //function-template-specialization
    class hash<FboCacheItem>
    {
    public :
        size_t operator()(const FboCacheItem &cacheItem) const
        {
            return hash<uint32_t>()(cacheItem.width) ^ hash<uint32_t>()(cacheItem.height);
        }
    };
};

NAMESPACE_RENDERCORE_BEGIN

class GLFrameBufferCache
{
public:
    GLFrameBufferPtr GetCacheFBO(const FboCacheItem& cacheItem) const;
    
    void AddFBO(const FboCacheItem& cacheItem, GLFrameBufferPtr fboPtr);
    
    static GLFrameBufferCache *getInstance();
    
private:
    std::unordered_map<FboCacheItem, GLFrameBufferPtr> mCachedFBOs;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_GL_FRAMEBUFFER_CACHE_INCKUDE_DGV */
