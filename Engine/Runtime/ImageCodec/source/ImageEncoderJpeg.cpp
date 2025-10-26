//
//  image_encoder_jpeg.cpp
//  
//
//  Created by Zhou,Xuguang on 2019/10/18.
//  Copyright © 2019年 zhouxuguang. All rights reserved.
//

#include "ImageEncoderJpeg.h"
#ifndef __APPLE__
#include "jpeglib.h"
#endif

#include <stdio.h>

#ifdef USE_JPEG_LIB

//待实现

NAMESPACE_IMAGECODEC_BEGIN

bool ImageEncoderJPEG::onEncode(std::vector<unsigned char>& dataStream, const VImage& image, int quality) const
{
#if 0
    unsigned int width = image.GetWidth();
    unsigned int height = image.GetHeight();
    unsigned int bytesPerPixel = image.GetBytesPerPixel();
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    /* 每一行的数据指针 */
    JSAMPROW row_pointer[1] = {0};
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

    /* Setting the parameters of the output file here */
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = bytesPerPixel;
    cinfo.in_color_space = color_space;
    /* default compression parameters, we shouldn't be worried about these */
    jpeg_set_defaults( &cinfo );
    /* Now do the compression .. */
    jpeg_start_compress( &cinfo, TRUE );
    /* like reading a file, this time write one row at a time */
    while (cinfo.next_scanline < cinfo.image_height)
    {
        row_pointer[0] = &raw_image[ cinfo.next_scanline * cinfo.image_width *  cinfo.input_components];
        jpeg_write_scanlines( &cinfo, row_pointer, 1 );
    }
    /* similar to read file, clean up after we're done compressing */
    jpeg_finish_compress( &cinfo );
    jpeg_destroy_compress( &cinfo );
    fclose( outfile );
#endif

    return false;
}

bool ImageEncoderJPEG::onEncodeFile(const char* fileName, const VImage& image, int quality) const
{
    if (nullptr == fileName)
    {
        return false;
    }

    FILE* fp = fopen(fileName, "wb");
    if (nullptr == fp)
    {
        return false;
    }

    std::vector<unsigned char> dataStream;
    bool bRet = onEncode(dataStream, image, quality);
    if (!bRet)
    {
        fclose(fp);
        return false;
    }

    size_t nWrituint8_ts = fwrite(dataStream.data(), 1, dataStream.size(), fp);
    if (nWrituint8_ts != dataStream.size())
    {
        fclose(fp);
        remove(fileName);
        return false;
    }

    fclose(fp);

    return true;
}

NAMESPACE_IMAGECODEC_END

#endif
