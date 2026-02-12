//
// Created by Zhou,Xuguang on 2015/10/22.
//

#include "VImage.h"
#include "Runtime/BaseLib/include/AlignedMalloc.h"
#include <stddef.h>

NAMESPACE_IMAGECODEC_BEGIN

static uint32_t GetFormatBytesPerRow(ImagePixelFormat format, uint32_t width, uint32_t height)
{
    switch (format)
    {
        case FORMAT_GRAY8:
        {
            return width;
        }
            
        case FORMAT_GRAY8_ALPHA8:
        case FORMAT_RGBA4444:
        case FORMAT_RGB5A1:
        case FORMAT_R5G6B5:
        {
            return width * 2;
        }
            
            
        case FORMAT_RGBA8:
        case FORMAT_SRGB8_ALPHA8:
        {
            return width * 4;
        }
            
        case FORMAT_RGB8:
        case FORMAT_SRGB8:
        {
            return width * 3;
        }

		case FORMAT_RGBA32Float:
		{
			return width * 16;
		}

		case FORMAT_RGB32Float:
		{
			return width * 12;
		}

		case FORMAT_RG16Float:
		{
			return width * 4;
		}

		case FORMAT_RG32Float:
		{
			return width * 8;
		}
            
        default:
            break;
    }
    
    return 0;
}

static uint32_t GetFormatBytes(ImagePixelFormat format)
{
	switch (format)
	{
	case FORMAT_GRAY8:
    {
        return 1;
    }
	case FORMAT_GRAY8_ALPHA8:
    {
        return 2;
    }

	case FORMAT_RGBA8:
	case FORMAT_SRGB8_ALPHA8:
	{
		return 4;
	}

	case FORMAT_RGB8:
	case FORMAT_SRGB8:
	{
		return 3;
	}

	case FORMAT_RGBA32Float:
	{
		return 16;
	}

	case FORMAT_RGB32Float:
	{
		return 12;
	}

	case FORMAT_RG16Float:
	{
		return 4;
	}

	case FORMAT_RG32Float:
	{
		return 8;
	}

	default:
		break;
	}

	return 0;
}

VImage::VImage(ImagePixelFormat format, uint32_t nWidth, uint32_t nHeight, const void *pData) :
                 mFormat(format),
                 mWidth(nWidth),
                 mHeight(nHeight),
                 mDeleteFunc(NULL),
                 mPremultipliedAlpha(false),
                 mMipCount(1)
{
    mData = (void *)pData;
    mBytesPerRow = GetFormatBytesPerRow(format, nWidth, nHeight);
    mBytesPerPixels = GetFormatBytes(format);
}

VImage::VImage()
{
    mFormat = FORMAT_UNKNOWN;
    mWidth = 0;
    mHeight = 0;
    mBytesPerRow = 0;
    mDeleteFunc = NULL;
    mData = NULL;
    mPremultipliedAlpha = false;
    mMipCount = 1;
}

VImage::~VImage()
{
    if (mDeleteFunc && mData)
    {
        mDeleteFunc(mData);
        mData = NULL;
    }

    mFormat = FORMAT_UNKNOWN;
    mWidth = 0;
    mHeight = 0;
    mBytesPerRow = 0;
    mDeleteFunc = NULL;
    mPremultipliedAlpha = false;
}

void VImage::AllocPixels()
{
    size_t nByteCount = mBytesPerRow * mHeight;
    if (nByteCount == 0)
    {
        return;
    }

    mData = baselib::AlignedMalloc(nByteCount, 64);
    mDeleteFunc = baselib::AlignedFree;
}

uint32_t VImage::GetWidth(int level) const
{
    return mWidth;
}

uint32_t VImage::GetHeight(int level) const
{
    return mHeight;
}

uint32_t VImage::GetBytesPerRow() const
{
    return mBytesPerRow;
}

uint32_t VImage::GetBytesPerPixels() const
{
    return mBytesPerPixels;
}

ImagePixelFormat VImage::GetFormat() const
{
    return mFormat;
}

uint8_t* VImage::GetPixels(int level) const
{
    return (uint8_t*)mData;
}

bool VImage::HasPremultipliedAlpha() const
{
    return mPremultipliedAlpha;
}

void VImage::SetImageInfo(ImagePixelFormat format, uint32_t nWidth, uint32_t nHeight)
{
    mFormat = format;
    mBytesPerRow = GetFormatBytesPerRow(mFormat, nWidth, nHeight);
    mBytesPerPixels = GetFormatBytes(mFormat);
    mWidth = nWidth;
    mHeight = nHeight;
    mData = NULL;
    mDeleteFunc = NULL;
    mMipCount = 1;
}

void VImage::SetImageInfo(ImagePixelFormat format, uint32_t nWidth, uint32_t nHeight, const void* pData, DeleteFun pDeleteFunc)
{
    mFormat = format;
    mBytesPerRow = GetFormatBytesPerRow(mFormat, nWidth, nHeight);
    mBytesPerPixels = GetFormatBytes(mFormat);
    mWidth = nWidth;
    mHeight = nHeight;
    mData = (void*)pData;
    
    mDeleteFunc = pDeleteFunc;
}

void VImage::SetPremultipliedAlpha(bool bPremultipliedAlpha)
{
    mPremultipliedAlpha = bPremultipliedAlpha;
}

void VImage::SetMipCount(int mipCount)
{
    mMipCount = mipCount;
}

int VImage::GetMipCount() const
{
    return mMipCount;
}

int VImage::GetImageSize(int level) const
{
    return mBytesPerRow * mHeight;
}

void VImage::Release()
{
    if (mDeleteFunc && mData)
    {
        mDeleteFunc(mData);
        mData = NULL;
    }
}

NAMESPACE_IMAGECODEC_END
