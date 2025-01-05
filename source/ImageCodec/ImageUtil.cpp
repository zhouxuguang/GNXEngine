//
// Created by Zhou,Xuguang on 2015/10/22.
//

#include "ImageUtil.h"

NAMESPACE_IMAGECODEC_BEGIN

ImageUtil::ImageUtil()
{
}

ImageUtil::~ImageUtil()
{
}

uint32_t ImageUtil::CalcNumMipLevels(uint32_t width, uint32_t height)
{
	uint32_t levels = 1;

	while ((width | height) >> levels)
		levels++;

	return levels;
}

NAMESPACE_IMAGECODEC_END