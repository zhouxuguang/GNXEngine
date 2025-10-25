//
// Created by Zhou,Xuguang on 2015/10/27.
//

#ifndef RENDERENGINE_IMAGEDECODERIMPL_H
#define RENDERENGINE_IMAGEDECODERIMPL_H

#include <stddef.h>
#include "VImage.h"

NAMESPACE_IMAGECODEC_BEGIN

bool hasAlphaChannel(ImagePixelFormat type);

void PremultipliedAlpha(unsigned char* pImateData, int width, int height, int bytesPerComponent);

class ImageDecoderImpl
{
public:
    /**
     * 从文件解码图像文件
     *
     * @param fileName android的assets目录下的文件必须以assets/开头
     * @param bitmap bitmap对象
     * @param format 存储格式
     * @return 返回值
     */
    static bool DecodeFile(const char *fileName,
                           VImage* bitmap,
                           ImageStoreFormat *format = NULL);

    /**
     * 从内存缓冲区解码图像
     *
     * @param buffer 内存对象
     * @param size 大小 字节
     * @param bitmap bitmap对象
     * @param format 存储格式
     * @return 返回值
     */
    static bool DecodeMemory(const void* buffer,
                             size_t size,
                             VImage* bitmap,
                             ImageStoreFormat* format = NULL);

    virtual ImageStoreFormat GetFormat() const;

    virtual bool IsFormat(const void* buffer, size_t size) = 0;

protected:
    virtual bool onDecode(const void* buffer, size_t size, VImage* bitmap) = 0;
};

typedef std::shared_ptr<ImageDecoderImpl> ImageDecoderImplPtr;

NAMESPACE_IMAGECODEC_END


#endif //RENDERENGINE_IMAGEDECODERIMPL_H
