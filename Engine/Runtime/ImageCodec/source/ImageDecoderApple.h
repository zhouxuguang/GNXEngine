//
//  image_decoder_apple.h
//  
//
//  Created by zhouxuguang on 2019/7/23.
//  Copyright © 2019 zhouxuguang. All rights reserved.
//

#ifndef IMAGEDECODER_APPLE_INCLUDE_H
#define IMAGEDECODER_APPLE_INCLUDE_H

#include "Define.h"
#include "ImageCodec/VImage.h"

NAMESPACE_IMAGECODEC_BEGIN

void DestroyCFDataRef(void* pData);

unsigned char* GetCFDataRef(void* pData);


uint8_t* DecodeImageData_APPLE(const uint8_t* pData, size_t dataLen, uint32_t* uiWidth, uint32_t* uiHeight, ImagePixelFormat &pixelFormat, bool& bPreMultyAlpha);

bool DecodeAppleImage(const void *buffer, size_t size, VImage *bitmap);

NAMESPACE_IMAGECODEC_END

#endif /* IMAGEDECODER_APPLE_INCLUDE_H */
