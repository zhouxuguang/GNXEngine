//
//  VertexData.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/20.
//

#include "VertexData.h"

NS_RENDERSYSTEM_BEGIN

VertexData::VertexData()
{
    //
}

VertexData::VertexData(uint32_t vertexCount, uint32_t vertexSize) : mVertexCount(vertexCount),
                                                            mVertexSize(vertexSize)
{
    mDataSize = mVertexSize * mVertexCount;
    mData = (uint8_t*)malloc(mDataSize);
}

VertexData::~VertexData()
{
    if (mData)
    {
        free(mData);
        mData = nullptr;
    }
}

bool VertexData::Resize(uint32_t vertexCount, uint32_t vertexSize)
{
    mVertexCount = vertexCount;
    mVertexSize = vertexSize;
    mDataSize = mVertexSize * mVertexCount;
    
    if (mData != nullptr)
    {
        mData = (uint8_t*)realloc(mData, mDataSize);
    }
    else
    {
        mData = (uint8_t*)malloc(mDataSize);
    }
    
    return true;
}

NS_RENDERSYSTEM_END
