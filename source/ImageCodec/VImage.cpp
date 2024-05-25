//
// Created by Zhou,Xuguang on 2015/10/22.
//

#include "VImage.h"
#include <stddef.h>

NAMESPACE_IMAGECODEC_BEGIN

static uint32_t GetFormatBytesPerRow(ImagePixelFormat format, uint32_t width, uint32_t height,
                                     uint32_t &encodedWidth, uint32_t &encodedHeight)
{
    switch (format)
    {
        case FORMAT_GRAY8:
        {
            encodedWidth = width;
            encodedHeight = height;
            return width;
        }
            
        case FORMAT_GRAY8_ALPHA8:
        case FORMAT_RGBA4444:
        case FORMAT_RGB5A1:
        case FORMAT_R5G6B5:
        {
            encodedWidth = width;
            encodedHeight = height;
            return width * 2;
        }
            
            
        case FORMAT_RGBA8:
        case FORMAT_SRGB8_ALPHA8:
        {
            encodedWidth = width;
            encodedHeight = height;
            return width * 4;
        }
            
        case FORMAT_RGB8:
        case FORMAT_SRGB8:
        {
            encodedWidth = width;
            encodedHeight = height;
            return width * 3;
        }
            
        case FORMAT_EAC_R:
        case FORMAT_EAC_R_SIGNED:
        case FORMAT_ETC2_RGB:
        case FORMAT_ETC2_SRGB:
        case FORMAT_ETC2_RGBA1:
        case FORMAT_ETC2_SRGBA1:
        case FORMAT_ETC1_RGB:
        {
            int xBlockCount = (width + 3) / 4;
            int yBlockCount = (height + 3) / 4;
            encodedWidth = xBlockCount * 4;
            encodedHeight = yBlockCount * 4;
            return xBlockCount * 8;
        }
            
        case FORMAT_EAC_RG:
        case FORMAT_EAC_RG_SIGNED:
        case FORMAT_ETC2_RGBA8:
        case FORMAT_ETC2_SRGBA8:
        {
            int xBlockCount = (width + 3) / 4;
            int yBlockCount = (height + 3) / 4;
            encodedWidth = xBlockCount * 4;
            encodedHeight = yBlockCount * 4;
            return xBlockCount * 16;
        }
            
        default:
            break;
    }
    
    return 0;
}

VImage::VImage(ImagePixelFormat format, uint32_t nWidth, uint32_t nHeight, const void *pData) :
                 m_eFormat(format),
                 m_nWidth(nWidth),
                 m_nHeight(nHeight),
                 m_pDeleteFunc(NULL),
                 m_bPremultipliedAlpha(false),
                 m_mipCount(1)
{
    m_pData = (void *)pData;
    m_nBytesPerRow = GetFormatBytesPerRow(format, nWidth, nHeight, m_nEncodedWidth, m_nEncodedHeight);
}

VImage::VImage()
{
    m_eFormat = FORMAT_UNKNOWN;
    m_nWidth = 0;
    m_nHeight = 0;
    m_nEncodedWidth = 0;
    m_nEncodedHeight = 0;
    m_nBytesPerRow = 0;
    m_pDeleteFunc = NULL;
    m_pData = NULL;
    m_bPremultipliedAlpha = false;
    m_mipCount = 1;
}

VImage::~VImage()
{
    if (m_pDeleteFunc && m_pData)
    {
        m_pDeleteFunc(m_pData);
        m_pData = NULL;
    }

    m_eFormat = FORMAT_UNKNOWN;
    m_nWidth = 0;
    m_nHeight = 0;
    m_nEncodedWidth = 0;
    m_nEncodedHeight = 0;
    m_nBytesPerRow = 0;
    m_pDeleteFunc = NULL;
    m_bPremultipliedAlpha = false;
}

void VImage::AllocPixels()
{
    size_t nByteCount = m_nBytesPerRow * m_nHeight;
    if (nByteCount == 0)
    {
        return;
    }

    m_pData = malloc(nByteCount);
    m_pDeleteFunc = free;
}

uint32_t VImage::GetWidth(int level) const
{
    if (level > 0)
    {
        return m_dataSizeAndOffsets[level].levelWidth;
    }
    return m_nWidth;
}

uint32_t VImage::GetHeight(int level) const
{
    if (level > 0)
    {
        return m_dataSizeAndOffsets[level].levelHeight;
    }
    return m_nHeight;
}

uint32_t VImage::GetBytesPerRow() const
{
    return m_nBytesPerRow;
}

ImagePixelFormat VImage::GetFormat() const
{
    return m_eFormat;
}

uint8_t* VImage::GetPixels(int level) const
{
    //TODO 这里需要重构
    uint8_t* data = (uint8_t*)m_pData;
    if (0 == level)
    {
        if (IsCompressedFormat(m_eFormat))
        {
            return data + m_dataSizeAndOffsets[0].offset;
        }
        else
        {
            return (uint8_t*)data;
        }
    }
    return data + m_dataSizeAndOffsets[level].offset;
}

bool VImage::HasPremultipliedAlpha() const
{
    return m_bPremultipliedAlpha;
}

void VImage::SetImageInfo(ImagePixelFormat format, uint32_t nWidth, uint32_t nHeight)
{
    m_eFormat = format;
    m_nBytesPerRow = GetFormatBytesPerRow(m_eFormat, nWidth, nHeight, m_nEncodedWidth, m_nEncodedHeight);
    m_nWidth = nWidth;
    m_nHeight = nHeight;
    m_pData = NULL;
    m_pDeleteFunc = NULL;
    m_mipCount = 1;
}

void VImage::SetImageInfo(ImagePixelFormat format, uint32_t nWidth, uint32_t nHeight, const void* pData, DeleteFun pDeleteFunc)
{
    m_eFormat = format;
    m_nBytesPerRow = GetFormatBytesPerRow(m_eFormat, nWidth, nHeight, m_nEncodedWidth, m_nEncodedHeight);
    m_nWidth = nWidth;
    m_nHeight = nHeight;
    m_pData = (void*)pData;
    
    m_pDeleteFunc = pDeleteFunc;
}

void VImage::SetPremultipliedAlpha(bool bPremultipliedAlpha)
{
    m_bPremultipliedAlpha = bPremultipliedAlpha;
}

void VImage::SetMipCount(int mipCount)
{
    m_mipCount = mipCount;
}

int VImage::GetMipCount() const
{
    return m_mipCount;
}

int VImage::GetImageSize(int level) const
{
    //TODO 这里也需要重构
    if (0 == level)
    {
        if (IsCompressedFormat(m_eFormat))
        {
            return m_dataSizeAndOffsets[0].dataSize;;
        }
        else
        {
            return m_nBytesPerRow * m_nHeight;
        }
    }
    return m_dataSizeAndOffsets[level].dataSize;
}

void VImage::SetMipDataSizeAndOffset(const std::vector<MipDataSizeAndOffset>& dataSizeAndOffset)
{
    m_dataSizeAndOffsets.clear();
    m_dataSizeAndOffsets.insert(m_dataSizeAndOffsets.end(), dataSizeAndOffset.begin(), dataSizeAndOffset.end());
}

void VImage::Release()
{
    if (m_pDeleteFunc && m_pData)
    {
        m_pDeleteFunc(m_pData);
        m_pData = NULL;
    }
}

NAMESPACE_IMAGECODEC_END
