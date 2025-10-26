//
// Created by Zhou,Xuguang on 2018/11/4.
//

#include "ImageDecoderBMP.h"
#include "libbmp/bitmap.h"
#include <new>

NAMESPACE_IMAGECODEC_BEGIN

bool ImageDecoderBMP::onDecode(const void *buffer, size_t size, VImage *bitmap)
{
    if (NULL == buffer || size <= 0 || NULL == bitmap)
    {
        return false;
    }
    CBitmap bitmap1;
    bool bRet = bitmap1.LoadFromMemory(buffer, (uint32_t)size);
    if (!bRet)
    {
        return false;
    }
//    bitmap->SetFormat(FORMAT_RGBA32);
//    bitmap->SetBitCount(32);
//    bitmap->SetPixels(bitmap1.GetBits());
//    bitmap->SetWidth(bitmap1.GetWidth());
//    bitmap->SetHeight(bitmap1.GetHeight());
//    bitmap->SetDeleteFunc(CBitmap::FreeBitmapData);

    bitmap1.ResetBitmap();
    return true;
}

ImageStoreFormat ImageDecoderBMP::GetFormat() const
{
    return kBMP_Format;
}

bool ImageDecoderBMP::IsFormat(const void *buffer, size_t size)
{

    return CBitmap::CheckBMP(buffer, (uint32_t)size);
}

ImageDecoderBMP* CreateBMPDecoder()
{
    return new(std::nothrow) ImageDecoderBMP();
}

NAMESPACE_IMAGECODEC_END
