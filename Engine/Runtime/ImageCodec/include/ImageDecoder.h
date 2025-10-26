//
// Created by Zhou,Xuguang on 2015/10/22.
//

#ifndef RENDERENGINE_IMAGELOADER_H
#define RENDERENGINE_IMAGELOADER_H

#include <stddef.h>
#include "VImage.h"

NAMESPACE_IMAGECODEC_BEGIN

class ImageDecoder
{
public:

    /**
     * 从文件解码图像文件
     *
     * @param fileName android的assets目录下的文件必须以assets/开头
     * @param bitmap 返回的图像数据
     * @param format 返回的图像存储格式
     * @return 是否加载成功
     */
    static bool DecodeFile(const char *fileName,
                           VImage* bitmap,
                           ImageStoreFormat *format = NULL);

    /**
     * 从内存缓冲区解码图像
     *
     * @param buffer 内存缓存
     * @param size 内存大小
     * @param bitmap 返回的图像数据
     * @param format 返回的图像存储格式
     * @return 是否加载成功
     */
    static bool DecodeMemory(const void* buffer,
                             size_t size,
                             VImage* bitmap,
                             ImageStoreFormat* format = NULL);

private:
    ImageDecoder();

    ~ImageDecoder();

    ImageDecoder(const ImageDecoder&);

    ImageDecoder& operator = (const ImageDecoder&);

};

NAMESPACE_IMAGECODEC_END


#endif //RENDERENGINE_IMAGELOADER_H
