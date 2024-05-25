//
//  image_encoder_apple.h
//  
//
//  Created by Zhou,Xuguang on 2019/10/18.
//  Copyright © 2019年 zhouxuguang. All rights reserved.
//

#ifndef RENDERENGINE_IMAGEENCODER_APPLE_HFHJDSFJD_INCLUDE_H
#define RENDERENGINE_IMAGEENCODER_APPLE_HFHJDSFJD_INCLUDE_H

#include "ImageCodec/VImage.h"
#include <ImageIO/ImageIO.h>
#include <vector>

NAMESPACE_IMAGECODEC_BEGIN

CGImageRef imageFormVImage(const VImage& image);

bool saveCGImageToFile(const char* fileName, CGImageRef imageRef, ImageStoreFormat format, int quality);

bool saveCGImageToBuffer(std::vector<unsigned char>& dataStream, CGImageRef imageRef, ImageStoreFormat format, int quality);

NAMESPACE_IMAGECODEC_END

#endif /* RENDERENGINE_IMAGEENCODER_APPLE_HFHJDSFJD_INCLUDE_H */
