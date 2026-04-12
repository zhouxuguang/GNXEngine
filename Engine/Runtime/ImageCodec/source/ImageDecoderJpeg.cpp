//
// Created by Zhou,Xuguang on 2015/11/2.
//

#include "ImageDecoderJpeg.h"
#include "ImageDecoderFactory.h"
#include "Runtime/BaseLib/include/AlignedMalloc.h"

#ifdef USE_JPEG_LIB
extern "C"
{
#include "libjpeg/jpeglib.h"
}

#endif

NAMESPACE_IMAGECODEC_BEGIN

#ifdef USE_JPEG_LIB

static uint8_t* DecodeJPEGData(const uint8_t* pJPEGData, size_t dataLen, uint32_t* uiWidth, uint32_t* uiHeight, uint32_t* uChannelCount, uint32_t* uBitCount, ImagePixelFormat &pixelFormat)
{
    /* these are standard libjpeg structures for reading(decompression) */
    struct jpeg_decompress_struct cInfo = {};
    struct jpeg_error_mgr jerr = {};

    uint8_t* pData = NULL;

    do
    {
        /* here we set up the standard libjpeg error handler */
        cInfo.err = jpeg_std_error(&jerr);
        /* setup decompression process and source, then read JPEG header */
        jpeg_create_decompress(&cInfo);

        if (NULL == cInfo.mem)
        {
            return NULL;
        }

        /* this makes the library read from infile */
        jpeg_mem_src(&cInfo, (uint8_t*)pJPEGData, dataLen);    //fuck，新版本的库为啥不支持内存解码了

        /* reading the image header which contains image information */
        jpeg_read_header(&cInfo, true);

        /* init image info */
        int width = cInfo.image_width;
        int height = cInfo.image_height;
        *uChannelCount = cInfo.num_components;
        *uBitCount = cInfo.num_components * 8;
        /* Start decompression jpeg here */
        jpeg_start_decompress(&cInfo);
        
        //解析图像格式，使用输出格式
        if (cInfo.out_color_space == JCS_RGB)
        {
            if (cInfo.num_components == 4)
            {
                pixelFormat = FORMAT_RGBA8;
            }
            
            else if (cInfo.num_components == 3)
            {
                pixelFormat = FORMAT_RGB8;
            }
        }
        
        else if (cInfo.out_color_space == JCS_GRAYSCALE)
        {
            if (cInfo.num_components == 2)
            {
                pixelFormat = FORMAT_GRAY8_ALPHA8;
            }
            
            else if (cInfo.num_components == 1)
            {
                pixelFormat = FORMAT_GRAY8;
            }
        }

        bool bError = false;
        int nWidthBytes = cInfo.image_width * cInfo.num_components;
        int nLen = height * nWidthBytes;
        uint8_t* pDataTmp = pData = (uint8_t*)baselib::AlignedMalloc(nLen, 64);

        if (pData == NULL)
        {
            return NULL;
        }

        for (int y = 0; y < height; y++)
        {
            jpeg_read_scanlines(&cInfo, (JSAMPARRAY)&pDataTmp , 1);

            pDataTmp += nWidthBytes;
        }

        jpeg_finish_decompress(&cInfo);
        jpeg_destroy_decompress(&cInfo);

        /* wrap up decompression, destroy objects, free pointers and close open files */
        if (bError)
        {
            baselib::AlignedFree(pData);
            pData = NULL;
        }

        // 输出描述信息
        *uiWidth = width;
        *uiHeight = height;
    }
    while (0);

    return pData;
}

bool ImageDecoderJPEG::onDecode(const void *buffer, size_t size, VImage *bitmap)
{
    uint32_t nWidth = 0;
    uint32_t nHeight = 0;
    uint32_t nBitCount = 0;
    uint32_t nChannelCount = 0;
    ImagePixelFormat pixelFormat = FORMAT_UNKNOWN;
    
    uint8_t* pData = DecodeJPEGData((const uint8_t*)buffer, size, &nWidth, &nHeight, &nChannelCount, &nBitCount, pixelFormat);
    if (!pData)
    {
        return false;
    }

    bitmap->SetImageInfo(pixelFormat, nWidth, nHeight, pData, baselib::AlignedFree);
    
    bool hasAlpha = hasAlphaChannel(pixelFormat);
    if (hasAlpha)
    {
        PremultipliedAlpha(pData, nWidth, nHeight, nChannelCount);
    }
    bitmap->SetPremultipliedAlpha(hasAlpha);
    return true;
}

ImageStoreFormat ImageDecoderJPEG::GetFormat() const
{
    return kJPEG_Format;
}

bool ImageDecoderJPEG::IsFormat(const void *buffer, size_t size)
{
    if (size < 10)
    {
        return false;
    }

    const uint8_t JPG_SOI[] = {0xFF, 0xD8};
    
    bool bFlag = memcmp(buffer, JPG_SOI, 2) == 0;
    if (!bFlag)
    {
        return false;
    }
    
    uint8_t* pJpegData = (uint8_t*)buffer;
    
    const uint8_t JFIF[] = {0x4A, 0x46, 0x49, 0x46};
    const uint8_t Exif[] = {0x45, 0x78, 0x69, 0x66};
    
    //如果都不是JFIF和Exif，可以认为不是jpeg文件
    if (memcmp(pJpegData + 6, JFIF, 4) && memcmp(pJpegData + 6, Exif, 4))
    {
        return false;
    }
    
    //EOI 的检查
    const uint8_t EOI[] = {0xFF, 0xD9};
    if (memcmp(pJpegData + size - 2, EOI, 2))
    {
        return false;
    }
    
    return true;
}

#endif

NAMESPACE_IMAGECODEC_END
