//
//  UniformBuffer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/1.
//

#ifndef GNX_ENGINE_UNIGORM_INCLUE_FSKGJNDGJN_HPP
#define GNX_ENGINE_UNIGORM_INCLUE_FSKGJNDGJN_HPP

#include "RenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

class UniformBuffer
{
public:
    UniformBuffer();
    
    virtual ~UniformBuffer();
    
    virtual void setData(const void* data, uint32_t offset, uint32_t dataSize) = 0;
};

typedef std::shared_ptr<UniformBuffer> UniformBufferPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_UNIGORM_INCLUE_FSKGJNDGJN_HPP */
