//
// Created by Zhou,Xuguang on 2015/10/25.
//

#include "ImageDecoderFactory.h"
#include "ImageDecoderPng.h"
#include "ImageDecoderJpeg.h"
#include "ImageDecoderBMP.h"
#include "ImageDecoderTGA.h"
#include "ImageDecoderWebp.h"
#include "ImageDecoderKTX.h"

NAMESPACE_IMAGECODEC_BEGIN

ImageDecoderFactory *ImageDecoderFactory::m_pInstance = NULL;
std::once_flag ImageDecoderFactory::m_OnceFlag;

ImageDecoderFactory* ImageDecoderFactory::GetInstance()
{
    std::call_once(m_OnceFlag, []()
    {
        m_pInstance = new(std::nothrow) ImageDecoderFactory();
        m_pInstance->AddImageDecoder(std::make_shared<ImageDecoderPNG>());
        m_pInstance->AddImageDecoder(std::make_shared<ImageDecoderJPEG>());
        m_pInstance->AddImageDecoder(std::make_shared<ImageDecoderBMP>());
        m_pInstance->AddImageDecoder(std::make_shared<ImageDecoderTGA>());
        m_pInstance->AddImageDecoder(std::make_shared<ImageDecoderWEBP>());
        m_pInstance->AddImageDecoder(std::make_shared<ImageDecoderKTX>());
    });

    return m_pInstance;
}

void ImageDecoderFactory::AddImageDecoder(ImageDecoderImplPtr pDecoder)
{
    mArrDecoders.push_back(pDecoder);
}

ImageDecoderImplPtr ImageDecoderFactory::GetImageDecoder(const void *buffer, size_t size)
{
    size_t nSize = mArrDecoders.size();
    for (size_t i = 0; i < nSize; ++i)
    {
        ImageDecoderImplPtr pDecoder = mArrDecoders[i];
        if (pDecoder && pDecoder->IsFormat(buffer, size))
        {
            return pDecoder;
        }
    }

    return NULL;
}

ImageDecoderFactory::ImageDecoderFactory()
{
    mArrDecoders.clear();
}

ImageDecoderFactory::~ImageDecoderFactory()
{
    mArrDecoders.clear();
}

NAMESPACE_IMAGECODEC_END
