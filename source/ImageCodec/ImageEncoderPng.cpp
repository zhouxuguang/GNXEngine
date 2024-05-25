//
//  image_encoder_png.cpp
//  
//
//  Created by Zhou,Xuguang on 2019/10/18.
//  Copyright © 2019年 zhouxuguang. All rights reserved.
//

#include "ImageEncoderPng.h"

#ifdef USE_PNG_LIB
#include "libpng/pngpriv.h"

NAMESPACE_IMAGECODEC_BEGIN

bool EncodeWithLibPNG(std::vector<unsigned char>& dataStream, const VImage& image,
                                     const bool hasAlpha, int colorType,
                                     int bitDepth, ImagePixelFormat format,
                                     png_color_8& sig_bit);

static bool formatHasAlpha(const ImagePixelFormat format)
{
    return false;
}

bool ImageEncoderPNG::onEncode(std::vector<unsigned char>& dataStream, const VImage& image, int quality) const
{
    const ImagePixelFormat format = image.GetFormat();
    const bool hasAlpha = formatHasAlpha(format);
    int colorType = 0;
    int bitDepth = 8;   // default for color
    png_color_8 sig_bit;
    memset(&sig_bit, 0, sizeof(png_color_8));
    
    switch (format)
    {
        case FORMAT_RGBA8:
            sig_bit.red = 8;
            sig_bit.green = 8;
            sig_bit.blue = 8;
            sig_bit.alpha = 8;
            colorType = PNG_COLOR_TYPE_RGB_ALPHA;
            break;

        case FORMAT_RGB8:
            sig_bit.red = 8;
            sig_bit.green = 8;
            sig_bit.blue = 8;
            sig_bit.alpha = 0;
            colorType = PNG_COLOR_TYPE_RGB;
            break;

        case FORMAT_RGBA4444:
            sig_bit.red = 4;
            sig_bit.green = 4;
            sig_bit.blue = 4;
            sig_bit.alpha = 4;
            colorType = PNG_COLOR_TYPE_RGB_ALPHA;
            break;

        case FORMAT_R5G6B5:
            sig_bit.red = 5;
            sig_bit.green = 6;
            sig_bit.blue = 5;
            sig_bit.alpha = 0;
            colorType = PNG_COLOR_TYPE_RGB;
            break;

        case FORMAT_RGB5A1:
            sig_bit.red = 5;
            sig_bit.green = 5;
            sig_bit.blue = 5;
            sig_bit.alpha = 1;
            colorType = PNG_COLOR_TYPE_RGB_ALPHA;
            break;

        case FORMAT_GRAY8:
            sig_bit.red = 0;
            sig_bit.green = 0;
            sig_bit.blue = 0;
            sig_bit.alpha = 0;
            sig_bit.gray = 8;
            colorType = PNG_COLOR_TYPE_GRAY;
            break;

        case FORMAT_GRAY8_ALPHA8:
            sig_bit.red = 0;
            sig_bit.green = 0;
            sig_bit.blue = 0;
            sig_bit.alpha = 8;
            sig_bit.gray = 8;
            colorType = PNG_COLOR_TYPE_GRAY_ALPHA;
            break;

        default:
            return false;
    }

    return EncodeWithLibPNG(dataStream, image, hasAlpha, colorType, bitDepth, format, sig_bit);
}

bool ImageEncoderPNG::onEncodeFile(const char* fileName, const VImage& image, int quality) const
{
//    stbi_write_png(fileName, image.GetWidth(), image.GetHeight(), 4, image.GetPixels(), image.GetWidth() * 4);
//    return true;
    
    if (!fileName)
    {
        return false;
    }
    
    remove(fileName);
    
    FILE* pFile = fopen(fileName, "wb");
    if (!pFile)
    {
        return false;
    }
    
    std::vector<unsigned char> dataStream;
    bool bRet = onEncode(dataStream, image, quality);
    if (!bRet)
    {
        fclose(pFile);
        return false;
    }
    
    size_t nWrituint8_ts = fwrite(dataStream.data(), 1, dataStream.size(), pFile);
    if (nWrituint8_ts != dataStream.size())
    {
        remove(fileName);
        fclose(pFile);
        return false;
    }
    
    fclose(pFile);
    return true;
}

static void libpng_encode_error_fn(png_structp png_ptr, png_const_charp msg)
{
    longjmp(png_jmpbuf(png_ptr), 1);
}

//写入操作的回掉函数
static void png_write_fn(png_structp png_ptr, png_bytep data, png_size_t len)
{
    std::vector<unsigned char>* pDataStream = (std::vector<unsigned char>*)png_get_io_ptr(png_ptr);
    if (!pDataStream)
    {
        return;
    }
    for (png_size_t i = 0; i < len; ++i)
    {
        pDataStream->push_back(data[i]);
    }
}

//颜色转换的函数

#define A4444_SHIFT    0
#define R4444_SHIFT    12
#define G4444_SHIFT    8
#define B4444_SHIFT    4

#define GetPackedA4444(c)     (((uint32_t)(c) >> A4444_SHIFT) & 0xF)
#define GetPackedR4444(c)     (((uint32_t)(c) >> R4444_SHIFT) & 0xF)
#define GetPackedG4444(c)     (((uint32_t)(c) >> G4444_SHIFT) & 0xF)
#define GetPackedB4444(c)     (((uint32_t)(c) >> B4444_SHIFT) & 0xF)

static inline uint32_t PackTo32(uint32_t nib)
{
    return (nib << 4) | nib;
}

#define Packed4444ToA32(c)    PackTo32(GetPackedA4444(c))
#define Packed4444ToR32(c)    PackTo32(GetPackedR4444(c))
#define Packed4444ToG32(c)    PackTo32(GetPackedG4444(c))
#define Packed4444ToB32(c)    PackTo32(GetPackedB4444(c))

typedef void (*transform_line_proc)(const unsigned char* src, int width, unsigned char* dst);

static void transform_scanline_565(const unsigned char* src, int width, unsigned char* dst)
{
    const uint16_t* srcP = (const uint16_t*)src;
    for (int i = 0; i < width; i++)
    {
        uint32_t rgb565 = *srcP++;
        uint8_t blue = (rgb565 & 0x001F) << 3;
        uint8_t green = (rgb565 & 0x07E0) >> 3;
        uint8_t red = (rgb565 & 0xF800) >> 8;
        
        *dst++ = red;
        *dst++ = green;
        *dst++ = blue;
    }
}

static void transform_scanline_4444(const unsigned char* src, int width, unsigned char* dst)
{
    const uint16_t* srcP = (const uint16_t*)src;
    for (int i = 0; i < width; i++)
    {
        uint16_t c = *srcP++;
        *dst++ = Packed4444ToR32(c);
        *dst++ = Packed4444ToG32(c);
        *dst++ = Packed4444ToB32(c);
        *dst++ = Packed4444ToA32(c);
    }
}

static void transform_scanline_5551(const unsigned char* src, int width, unsigned char* dst)
{
    //待实现
    const uint16_t* srcP = (const uint16_t*)src;
    for (int i = 0; i < width; i++)
    {
        unsigned int c = *srcP++;
    }
}

static void transform_scanline_888(const unsigned char* src, int width, unsigned char* dst)
{
    memcpy(dst, src, width * 3);
}

static void transform_scanline_8888(const unsigned char* src, int width, unsigned char* dst)
{
//    unsigned char* pSrc = (unsigned char*)src;
//    for (int i = 0; i < width; i++)
//    {
//        uint32_t r = *pSrc++;
//        uint32_t g = *pSrc++;
//        uint32_t b = *pSrc++;
//        uint32_t a = *pSrc++;
//
//        if (a != 0)
//        {
//            float invAlpha = 255.0f / a;
//            r *= invAlpha;
//            g *= invAlpha;
//            b *= invAlpha;
//        }
//
//        *dst++ = r;
//        *dst++ = g;
//        *dst++ = b;
//        *dst++ = a;
//    }
    memcpy(dst, src, width * 4);
}

static void transform_scanline_8(const unsigned char* src, int width, unsigned char* dst)
{
    memcpy(dst, src, width);
}

static void transform_scanline_88(const unsigned char* src, int width, unsigned char* dst)
{
    memcpy(dst, src, width * 2);
}

static transform_line_proc choose_tranform_proc(ImagePixelFormat format, bool hasAlpha)
{
    switch (format)
    {
        case FORMAT_RGBA8:
            return transform_scanline_8888;

        case FORMAT_RGB8:
            return transform_scanline_888;

        case FORMAT_RGBA4444:
            return transform_scanline_4444;

        case FORMAT_R5G6B5:
            return transform_scanline_565;

        case FORMAT_RGB5A1:
            return transform_scanline_5551;

        case FORMAT_GRAY8:
            return transform_scanline_8;

        case FORMAT_GRAY8_ALPHA8:
            return transform_scanline_88;

        default:
            return NULL;
    }
}

bool EncodeWithLibPNG(std::vector<unsigned char>& dataStream, const VImage& image,
                        const bool hasAlpha, int colorType,
                        int bitDepth, ImagePixelFormat format,
                        png_color_8& sig_bit)
{
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, libpng_encode_error_fn, NULL);
    if (NULL == png_ptr)
    {
        return false;
    }
    
    info_ptr = png_create_info_struct(png_ptr);
    if (NULL == info_ptr)
    {
        png_destroy_write_struct(&png_ptr, NULL);
        return false;
    }
    
    //错误处理
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return false;
    }
    
    //设置写入函数
    png_set_write_fn(png_ptr, &dataStream, png_write_fn, NULL);

    //预先分配内存
    dataStream.reserve(image.GetHeight() * image.GetWidth() * image.GetBytesPerPixel() / 3);
    
    /* 设置图像的宽、高、格式、压缩方式等基本信息
     */
    png_set_IHDR(png_ptr, info_ptr, image.GetWidth(), image.GetHeight(),
                 bitDepth, colorType,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
                 PNG_FILTER_TYPE_BASE);
    
    //设置位的信息
    png_set_sBIT(png_ptr, info_ptr, &sig_bit);
    png_set_packing(png_ptr);
    png_write_info(png_ptr, info_ptr);
    
    size_t nPerLinuint8_ts = image.GetWidth() * image.GetBytesPerPixel();
    unsigned char* srcImage =  image.GetPixels() + (image.GetHeight() - 1) * nPerLinuint8_ts;
    unsigned char* storage = (unsigned char*)malloc(image.GetWidth() * 4);
    transform_line_proc transformProc = choose_tranform_proc(format, hasAlpha);
    
    //实际的写入操作
    for (int y = image.GetHeight() - 1; y >= 0; y--)
    {
        transformProc(srcImage, image.GetWidth(), storage);
        png_write_rows(png_ptr, &storage, 1);
        srcImage -= nPerLinuint8_ts;
    }
    
    png_write_end(png_ptr, info_ptr);
    free(storage);
    
    /* 清除相关的内存 */
    png_destroy_write_struct(&png_ptr, &info_ptr);

    dataStream.shrink_to_fit();

    return true;
}

NAMESPACE_IMAGECODEC_END

#endif
