//
//  ImageUtil.h
//  GNXEngine
//
//  Created by Zhou,Xuguang on 2019/10/18.
//  Copyright © 2019年 zhouxuguang. All rights reserved.
//

#ifndef RENDERENGINE_IMAGE_ENCODER_IMAGE_UTILS_INCLUDE_SDGDFJGN
#define RENDERENGINE_IMAGE_ENCODER_IMAGE_UTILS_INCLUDE_SDGDFJGN

#include "Define.h"

NAMESPACE_IMAGECODEC_BEGIN

// 图像处理的工具类
class ImageUtil
{
public:
	ImageUtil();
	~ImageUtil();

	// 计算mipmap的等级
	static uint32_t CalcNumMipLevels(uint32_t width, uint32_t height);

private:
};

NAMESPACE_IMAGECODEC_END

#endif