//
// Created by Zhou,Xuguang on 2015/10/22.
//

#include <new>
#include <memory.h>
#include "ImageDecoderPng.h"

#ifdef USE_PNG_LIB
#include "libpng/pngpriv.h"
#endif

NAMESPACE_IMAGECODEC_BEGIN

#ifdef USE_PNG_LIB

static void pngtest_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    //png_size_t check = 0;
    png_voidp io_ptr;
    /* fread() returns 0 on error, so it is OK to store this in a png_size_t
     * instead of an int, which is what fread() actually returns.
     */
    io_ptr = png_get_io_ptr(png_ptr);
    
    if (io_ptr != NULL)
    {
        //check = fread(data, 1, length, (png_FILE_p)io_ptr);
        memcpy(data, io_ptr, length);
    }
    
    png_ptr->io_ptr = (char*)png_ptr->io_ptr + length;
}

static uint8_t* DecodePngData(const uint8_t* pPngData, size_t dataLen, uint32_t* uiWidth, uint32_t* uiHeight, uint32_t* uChannelCount, uint32_t* uBitCount, ImagePixelFormat &pixelFormat)
{
    if (NULL == pPngData || 0 == dataLen)
    {
        return NULL;
    }
    
#define PNGSIGSIZE  8
    bool ret = false;
    png_structp png_ptr = 0;
    png_infop info_ptr = 0;
    uint8_t* pData = NULL;
    
    do
    {
        // png header len is 8 bytes
        if (dataLen < PNGSIGSIZE)
        {
            return NULL;
        }
        
        // check the data is png or not
        if (png_sig_cmp((png_const_bytep)pPngData, 0, PNGSIGSIZE))
        {
            return NULL;
        }
        
        // init png_struct
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
        
        // init png_info
        info_ptr = png_create_info_struct(png_ptr);
        
        setjmp(png_jmpbuf(png_ptr));
        
        // set the read call back function
        png_set_read_fn(png_ptr, (png_voidp)pPngData, pngtest_read_data);
        
        // read png file info
        png_read_info(png_ptr, info_ptr);
        
        png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
        png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
        png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);
        png_uint_32 color_type = png_get_color_type(png_ptr, info_ptr);
        
        // force palette images to be expanded to 24-bit RGB
        // it may include alpha channel
        if (color_type == PNG_COLOR_TYPE_PALETTE)
        {
            png_set_palette_to_rgb(png_ptr);
        }
        // low-bit-depth grayscale images are to be expanded to 8 bits
        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        {
            bit_depth = 8;
            png_set_expand_gray_1_2_4_to_8(png_ptr);
        }
        // expand any tRNS chunk data into a full alpha channel
        //        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        //        {
        //            png_set_tRNS_to_alpha(png_ptr);
        //        }
        // reduce images with 16-bit samples to 8 bits
        if (bit_depth == 16)
        {
            png_set_strip_16(png_ptr);
        }
        
        // Expanded earlier for grayscale, now take care of palette and rgb
        if (bit_depth < 8)
        {
            png_set_packing(png_ptr);
        }
        // update info
        png_read_update_info(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);
        int renderIntent = 0;
        png_uint_32 sRGBModel = png_get_sRGB(png_ptr, info_ptr, &renderIntent);
        bool isSRGB = false;
        if (PNG_INFO_sRGB == sRGBModel)
        {
            isSRGB = true;
        }
        
        // read png data
        png_size_t rowbytes;
        png_bytep* row_pointers = (png_bytep*)malloc( sizeof(png_bytep) * height );
        
        rowbytes = png_get_rowbytes(png_ptr, info_ptr);
        
        size_t LenOfBytes = rowbytes * height;
        pData = (uint8_t*)malloc(LenOfBytes);
        if (!pData)
        {
            if (row_pointers != nullptr)
            {
                free(row_pointers);
            }
            break;
        }
        
        for (unsigned int i = 0; i < height; ++i)
        {
            row_pointers[i] = pData + i * rowbytes;
        }
        png_read_image(png_ptr, row_pointers);
        
        png_read_end(png_ptr, nullptr);
        
        if (row_pointers != nullptr)
        {
            free(row_pointers);
        }
        
        *uiWidth = width;
        *uiHeight = height;
        *uBitCount = info_ptr->pixel_depth;
        
        //解析像素格式
        if (color_type == PNG_COLOR_TYPE_GRAY)
        {
            pixelFormat = FORMAT_GRAY8;
            *uChannelCount = 1;
        }
        
        else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        {
            pixelFormat = FORMAT_GRAY8_ALPHA8;
            *uChannelCount = 2;
        }
        
        else if (color_type == PNG_COLOR_TYPE_RGB)
        {
            pixelFormat = FORMAT_RGB8;
            if (isSRGB)
            {
                pixelFormat = FORMAT_SRGB8;
            }
            *uChannelCount = 3;
        }
        
        else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
        {
            pixelFormat = FORMAT_RGBA8;
            if (isSRGB)
            {
                pixelFormat = FORMAT_SRGB8_ALPHA8;
            }
            *uChannelCount = 4;
        }
        
        ret = true;
    } while (0);
    
    if (png_ptr)
    {
        png_destroy_read_struct(&png_ptr, (info_ptr) ? &info_ptr : 0, 0);
    }
    return pData;
}

bool ImageDecoderPNG::onDecode(const void *buffer, size_t size, VImage *bitmap)
{
    if (NULL == bitmap)
    {
        return false;
    }
    
    uint32_t nWidth = 0;
    uint32_t nHeight = 0;
    uint32_t nBitCount = 0;
    uint32_t nChannelCount = 0;
    ImagePixelFormat pixelFormat = FORMAT_UNKNOWN;
    
    uint8_t* pData = DecodePngData((const uint8_t*)buffer, size, &nWidth, &nHeight, &nChannelCount, &nBitCount, pixelFormat);
    if (!pData)
    {
        return false;
    }
    bitmap->SetImageInfo(pixelFormat, nWidth, nHeight, pData, free);
    
    bool hasAlpha = hasAlphaChannel(pixelFormat);
    if (hasAlpha && bitmap->HasPremultipliedAlpha())
    {
        PremultipliedAlpha(pData, nWidth, nHeight, nChannelCount);
    }
   // bitmap->SetPremultipliedAlpha(hasAlpha);
    return true;
}

ImageStoreFormat ImageDecoderPNG::GetFormat() const
{
    return kPNG_Format;
}

static int pngHeaderCheck(const unsigned char* sig, size_t start, size_t num_to_check)
{
    unsigned char png_signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};

    if (num_to_check > 8)
        num_to_check = 8;

    else if (num_to_check < 1)
        return (-1);

    if (start > 7)
        return (-1);

    if (start + num_to_check > 8)
        num_to_check = 8 - start;

    return ((int)(memcmp(&sig[start], &png_signature[start], num_to_check)));
}

bool ImageDecoderPNG::IsFormat(const void *buffer, size_t size)
{
    int is_png = pngHeaderCheck((const unsigned char*)buffer, 0, 8);
    return is_png == 0;
}

#endif

NAMESPACE_IMAGECODEC_END
