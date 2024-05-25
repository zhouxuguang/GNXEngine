//
//  IndexBuffer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/27.
//

#ifndef RENDERCORE_INDEXBUFFER_HPP_INCLUDE_HJHSSD
#define RENDERCORE_INDEXBUFFER_HPP_INCLUDE_HJHSSD

#include "RenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

// 索引buffer
class IndexBuffer
{
public:
    IndexBuffer(IndexType indexType, const void* pData, uint32_t dataLen);
    
    virtual ~IndexBuffer();
};

typedef std::shared_ptr<IndexBuffer> IndexBufferPtr;

NAMESPACE_RENDERCORE_END

#endif /* RENDERCORE_INDEXBUFFER_HPP_INCLUDE_HJHSSD */
