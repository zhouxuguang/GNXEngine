//
//  image_encoder_jpeg.cpp
//  
//
//  Created by Zhou,Xuguang on 2019/10/18.
//  Copyright © 2019年 zhouxuguang. All rights reserved.
//

#include "ImageEncoderJpeg.h"

#ifdef USE_JPEG_LIB
extern "C"
{
#include "libjpeg/jpeglib.h"
}
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_JPEG_LIB

NAMESPACE_IMAGECODEC_BEGIN

// 自定义内存输出管理器
#define OUTPUT_BUF_SIZE 4096

typedef struct 
{
    struct jpeg_destination_mgr pub;
    std::vector<unsigned char>* output;
    JOCTET* buffer;
} mem_destination_mgr;

typedef mem_destination_mgr* mem_dest_ptr;

// 初始化目标
static void init_mem_destination(j_compress_ptr cinfo)
{
    mem_dest_ptr dest = (mem_dest_ptr)cinfo->dest;
    dest->buffer = (JOCTET*)malloc(OUTPUT_BUF_SIZE);
    if (dest->buffer == nullptr) 
    {
        //ERREXIT1(cinfo, JERR_OUT_OF_MEMORY, 10);
    }
    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

// 缓冲区满时调用
static boolean empty_mem_output_buffer(j_compress_ptr cinfo)
{
    mem_dest_ptr dest = (mem_dest_ptr)cinfo->dest;
    
    // 将缓冲区数据追加到输出 vector
    size_t oldsize = dest->output->size();
    dest->output->resize(oldsize + OUTPUT_BUF_SIZE);
    memcpy(dest->output->data() + oldsize, dest->buffer, OUTPUT_BUF_SIZE);
    
    // 重置缓冲区
    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
    
    return TRUE;
}

// 结束时调用
static void term_mem_destination(j_compress_ptr cinfo)
{
    mem_dest_ptr dest = (mem_dest_ptr)cinfo->dest;
    size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;
    
    // 将剩余数据追加到输出 vector
    if (datacount > 0) 
    {
        size_t oldsize = dest->output->size();
        dest->output->resize(oldsize + datacount);
        memcpy(dest->output->data() + oldsize, dest->buffer, datacount);
    }
    
    // 释放缓冲区
    free(dest->buffer);
    dest->buffer = nullptr;
}

// 设置内存输出目标
static void jpeg_mem_dest_custom(j_compress_ptr cinfo, std::vector<unsigned char>* output)
{
    mem_dest_ptr dest;
    
    if (cinfo->dest == nullptr) 
    {
        cinfo->dest = (struct jpeg_destination_mgr*)
            (*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT,
                                        sizeof(mem_destination_mgr));
    }
    
    dest = (mem_dest_ptr)cinfo->dest;
    dest->pub.init_destination = init_mem_destination;
    dest->pub.empty_output_buffer = empty_mem_output_buffer;
    dest->pub.term_destination = term_mem_destination;
    dest->output = output;
}

bool ImageEncoderJPEG::onEncode(std::vector<unsigned char>& dataStream, const VImage& image, int quality) const
{
    unsigned int width = image.GetWidth();
    unsigned int height = image.GetHeight();
    ImagePixelFormat format = image.GetFormat();
    uint8_t* raw_data = image.GetImageData();

    if (!raw_data || width == 0 || height == 0)
    {
        return false;
    }

    // 根据图像格式确定颜色空间和分量数
    J_COLOR_SPACE color_space;
    int components;
    bool needConvert = false;  // 是否需要去除 Alpha 通道

    switch (format)
    {
        case FORMAT_GRAY8:
            color_space = JCS_GRAYSCALE;
            components = 1;
            break;
        case FORMAT_RGB8:
        case FORMAT_SRGB8:
            color_space = JCS_RGB;
            components = 3;
            break;
        case FORMAT_RGBA8:
        case FORMAT_SRGB8_ALPHA8:
            // JPEG 不支持 Alpha，需转换为 RGB
            color_space = JCS_RGB;
            components = 3;
            needConvert = true;
            break;
        default:
            return false;  // 不支持的格式
    }

    // 清空输出缓冲区
    dataStream.clear();

    // 初始化 JPEG 压缩结构
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    // 设置自定义内存输出目标
    jpeg_mem_dest_custom(&cinfo, &dataStream);

    // 设置图像参数
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = components;
    cinfo.in_color_space = color_space;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    // 开始压缩
    jpeg_start_compress(&cinfo, TRUE);

    // 准备行缓冲区
    int row_stride = width * components;

    if (needConvert)
    {
        // RGBA -> RGB 转换
        std::vector<uint8_t> rowBuffer(row_stride);
        JSAMPROW row_pointer[1];

        while (cinfo.next_scanline < cinfo.image_height)
        {
            uint8_t* srcRow = raw_data + (size_t)cinfo.next_scanline * width * 4;
            // RGBA -> RGB
            for (unsigned int i = 0; i < width; i++)
            {
                rowBuffer[i * 3 + 0] = srcRow[i * 4 + 0];
                rowBuffer[i * 3 + 1] = srcRow[i * 4 + 1];
                rowBuffer[i * 3 + 2] = srcRow[i * 4 + 2];
            }
            row_pointer[0] = rowBuffer.data();
            jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }
    }
    else
    {
        // 直接写入
        JSAMPROW row_pointer[1];
        while (cinfo.next_scanline < cinfo.image_height)
        {
            row_pointer[0] = raw_data + (size_t)cinfo.next_scanline * row_stride;
            jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    return true;
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

    size_t nWrites = fwrite(dataStream.data(), 1, dataStream.size(), fp);
    if (nWrites != dataStream.size())
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
