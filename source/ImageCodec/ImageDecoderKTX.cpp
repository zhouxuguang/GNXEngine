//
//  ImageDecoderKTX.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/5/14.
//

#include "ImageDecoderKTX.h"
#include "BytesSwap.h"

NAMESPACE_IMAGECODEC_BEGIN

typedef struct// __attribute__((packed))
{
    uint8_t identifier[12];
    uint32_t endianness;
    uint32_t glType;
    uint32_t glTypeSize;
    uint32_t glFormat;
    uint32_t glInternalFormat;
    uint32_t glBaseInternalFormat;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t arrayElementCount;
    uint32_t faceCount;
    uint32_t mipmapCount;
    uint32_t keyValueDataLength;
} MBEKTXHeader;

#define GL_COMPRESSED_R11_EAC                            0x9270
#define GL_COMPRESSED_SIGNED_R11_EAC                     0x9271
#define GL_COMPRESSED_RG11_EAC                           0x9272
#define GL_COMPRESSED_SIGNED_RG11_EAC                    0x9273
#define GL_COMPRESSED_RGB8_ETC2                          0x9274
#define GL_COMPRESSED_SRGB8_ETC2                         0x9275
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2      0x9276
#define GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2     0x9277
#define GL_COMPRESSED_RGBA8_ETC2_EAC                     0x9278
#define GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC              0x9279

#ifndef ETC1_RGB8_OES
#define ETC1_RGB8_OES 0x8D64

#define GL_RGBA8 0X8058

#endif

ImagePixelFormat PixelFormatForGLInternalFormat(uint32_t internalFormat)
{
    switch (internalFormat)
    {
        case GL_COMPRESSED_R11_EAC:
            return FORMAT_EAC_R;
        case GL_COMPRESSED_SIGNED_R11_EAC:
            return FORMAT_EAC_R_SIGNED;
        case GL_COMPRESSED_RG11_EAC:
            return FORMAT_EAC_RG;
        case GL_COMPRESSED_SIGNED_RG11_EAC:
            return FORMAT_EAC_RG_SIGNED;
        case GL_COMPRESSED_RGB8_ETC2:
            return FORMAT_ETC2_RGB;
        case GL_COMPRESSED_SRGB8_ETC2:
            return FORMAT_ETC2_SRGB;
        case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
            return FORMAT_ETC2_RGBA1;
        case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
            return FORMAT_ETC2_SRGBA1;
        case GL_COMPRESSED_RGBA8_ETC2_EAC:
            return FORMAT_ETC2_RGBA8;
        case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
            return FORMAT_ETC2_SRGBA8;
            
        case GL_RGBA8:
            return FORMAT_RGBA8;
            
        case 33327:
            return FORMAT_SRGB8_ALPHA8;
            
        case ETC1_RGB8_OES:
            return FORMAT_ETC1_RGB;
        default:
            return FORMAT_UNKNOWN;
    }
}

bool ImageDecoderKTX::onDecode(const void* buffer, size_t size, VImage* bitmap)
{
    if (size <= 12 || nullptr == buffer || bitmap == nullptr)
    {
        return false;
    }
    
    MBEKTXHeader *header = (MBEKTXHeader *)buffer;
    bool endianSwap = (header->endianness == 0x01020304);

    uint32_t width = endianSwap ? baselib::BytesSwap::SwapBytes(header->width) : header->width;
    uint32_t height = endianSwap ? baselib::BytesSwap::SwapBytes(header->height) : header->height;
    uint32_t internalFormat = endianSwap ? baselib::BytesSwap::SwapBytes(header->glInternalFormat) : header->glInternalFormat;
    uint32_t mipCount = endianSwap ? baselib::BytesSwap::SwapBytes(header->mipmapCount) : header->mipmapCount;
    uint32_t keyValueDataLength = endianSwap ? baselib::BytesSwap::SwapBytes(header->keyValueDataLength) : header->keyValueDataLength;
    
    ImagePixelFormat imageFormat = PixelFormatForGLInternalFormat(internalFormat);
    if (imageFormat == FORMAT_UNKNOWN)
    {
        return false;
    }

    const uint8_t *bytes = (const uint8_t*)buffer + sizeof(MBEKTXHeader) + keyValueDataLength;
    const size_t dataLength = size - (sizeof(MBEKTXHeader) + keyValueDataLength);
    
    mipCount = std::max(mipCount, 1u);

    //计算各个mipleve的大小和偏移
    uint32_t dataOffset = 0;
    uint32_t levelWidth = width, levelHeight = height;
    
    std::vector<MipDataSizeAndOffset> dataSizeAndOffset;
    
    while (dataOffset < dataLength)
    {
        uint32_t levelSize = *(uint32_t *)(bytes + dataOffset);
        dataOffset += sizeof(uint32_t);
        
        MipDataSizeAndOffset mipDataSizeAndOffset;
        mipDataSizeAndOffset.dataSize = levelSize;
        mipDataSizeAndOffset.offset = dataOffset;
        mipDataSizeAndOffset.levelWidth = levelWidth;
        mipDataSizeAndOffset.levelHeight = levelHeight;
        dataSizeAndOffset.push_back(mipDataSizeAndOffset);

        dataOffset += levelSize;

        levelWidth = std::max(levelWidth / 2, 1u);
        levelHeight = std::max(levelHeight / 2, 1u);
    }
    
    if (dataSizeAndOffset.size() != mipCount)
    {
        return false;
    }
    
    uint8_t* pCopyData = (uint8_t*)malloc(dataLength);
    memcpy(pCopyData, bytes, dataLength);
    
    bitmap->SetImageInfo(imageFormat, width, height, pCopyData, free);
    bitmap->SetMipCount(mipCount);
    bitmap->SetMipDataSizeAndOffset(dataSizeAndOffset);
    
    return true;
}

bool ImageDecoderKTX::IsFormat(const void* buffer, size_t size)
{
    if (size <= 12 || nullptr == buffer)
    {
        return false;
    }
    
    uint8_t fileIdentifier[12] = {
       0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
    };
//    This can also be expressed using C-style character definitions as:
//
//    Byte[12] FileIdentifier = {
//        '«', 'K', 'T', 'X', ' ', '1', '1', '»', '\r', '\n', '\x1A', '\n'
//    }
    return 0 == memcmp(fileIdentifier, buffer, 12);
}

ImageStoreFormat ImageDecoderKTX::GetFormat() const
{
    return kUnknown_Format;
}

NAMESPACE_IMAGECODEC_END
