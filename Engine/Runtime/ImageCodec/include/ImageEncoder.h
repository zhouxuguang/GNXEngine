//
//  image_encoder.h
//  GNXEngine
//
//  Created by Zhou,Xuguang on 2019/10/18.
//  Copyright © 2019年 zhouxuguang. All rights reserved.
//

#ifndef IMAGE_DECODER_INCLUDE_HDKFHKS_H
#define IMAGE_DECODER_INCLUDE_HDKFHKS_H

#include "VImage.h"
#include <vector>

NAMESPACE_IMAGECODEC_BEGIN

class ImageEncoder
{
public:
    /**
     * 将图像数据写入文件，即编码
     *
     * @param fileName 存储的路径
     * @param image 要编码的图像数据
     * @param format 图像的存储格式
     * @param quality 压缩的质量0-100， 100是质量最好的
     * @return 是否编码成功
     */
    static bool EncodeFile(const char *fileName, const VImage& image, ImageStoreFormat format, int quality);
    
    /**
     * 将图像数据写入二进制流，即编码
     *
     * @param dataStream 编码后的二进制数据
     * @param image 要编码的图像数据
     * @param format 图像的存储格式
     * @param quality 压缩的质量0-100， 100是质量最好的
     * @return 是否编码成功
     */
    static bool EncodeMemory(std::vector<unsigned char>& dataStream, const VImage& image, ImageStoreFormat format, int quality);
    
private:
    ImageEncoder();
    
    ~ImageEncoder();
    
    ImageEncoder(const ImageEncoder&);
    
    ImageEncoder& operator = (const ImageEncoder&);
};

NAMESPACE_IMAGECODEC_END

#endif /* IMAGE_DECODER_INCLUDE_HDKFHKS_H */
