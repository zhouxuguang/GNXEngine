//
// Created by Zhou,Xuguang on 2015/11/2.
//

#include "ImageDecoderJpeg.h"
#include "ImageDecoderFactory.h"

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
    struct jpeg_decompress_struct cinfo;
    //cinfo.error_map = 0;
    struct jpeg_error_mgr jerr;

    uint8_t* pData = NULL;

    do
    {
        /* here we set up the standard libjpeg error handler */
        cinfo.err = jpeg_std_error(&jerr);
        /* setup decompression process and source, then read JPEG header */
        jpeg_create_decompress(&cinfo);

//        if (cinfo.error_map != 0)
//            return NULL;

        if (NULL == cinfo.mem)
        {
            return NULL;
        }

        /* this makes the library read from infile */
        jpeg_mem_src(&cinfo, (uint8_t*)pJPEGData, dataLen);    //fuck，新版本的库为啥不支持内存解码了
        //jpeg_stdio_src(j_decompress_ptr cinfo, FILE *infile)

//        if (cinfo.error_map != 0)
//        {
//            return NULL;
//        }

        /* reading the image header which contains image information */
        jpeg_read_header(&cinfo, true);

//        if (cinfo.error_map != 0)
//        {
//            return NULL;
//        }

        /* init image info */
        int width = cinfo.image_width;
        int height = cinfo.image_height;
        *uChannelCount = cinfo.num_components;
        *uBitCount = cinfo.num_components * 8;
        /* Start decompression jpeg here */
        jpeg_start_decompress(&cinfo);
        
        //解析图像格式，使用输出格式
        if (cinfo.out_color_space == JCS_RGB)
        {
            if (cinfo.num_components == 4)
            {
                pixelFormat = FORMAT_SRGB8_ALPHA8;
            }
            
            else if (cinfo.num_components == 3)
            {
                pixelFormat = FORMAT_SRGB8;
            }
        }
        
        else if (cinfo.out_color_space == JCS_GRAYSCALE)
        {
            if (cinfo.num_components == 2)
            {
                pixelFormat = FORMAT_GRAY8_ALPHA8;
            }
            
            else if (cinfo.num_components == 1)
            {
                pixelFormat = FORMAT_GRAY8;
            }
        }

//        if (cinfo.error_map != 0)
//        {
//            return NULL;
//        }

        bool bError = false;
        int nWidthBytes = cinfo.image_width * cinfo.num_components;
        int nLen = height * nWidthBytes;
        uint8_t* pDataTmp = pData = (uint8_t*)malloc(nLen);

        if (pData == NULL)
        {
            return NULL;
        }

        for (int y = 0; y < height; y++)
        {
            jpeg_read_scanlines(&cinfo, (JSAMPARRAY)&pDataTmp , 1);

//            if (cinfo.error_map != 0)
//            {
//                bError = true;
//                break;
//            }

            pDataTmp += nWidthBytes;
        }


        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);

        /* wrap up decompression, destroy objects, free pointers and close open files */
        if (bError)
        {
            free(pData);
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
    bitmap->SetImageInfo(pixelFormat, nWidth, nHeight, pData, free);
    
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
