//
// Created by Zhou,Xuguang on 2015/10/25.
//

#ifndef RENDERENGINE_IMAGEDECODERFACTORY_H
#define RENDERENGINE_IMAGEDECODERFACTORY_H

#include <vector>
#include <mutex>
#include <memory>
#include "ImageDecoderImpl.h"

NAMESPACE_IMAGECODEC_BEGIN

class ImageDecoderFactory
{
public:
    static ImageDecoderFactory* GetInstance();

    //找不到返回空
    ImageDecoderImplPtr GetImageDecoder(const void* buffer, size_t size);

    //添加图像驱动
    void AddImageDecoder(ImageDecoderImplPtr pDecoder);

private:
    std::vector<ImageDecoderImplPtr> mArrDecoders;            //解码器集合
    static ImageDecoderFactory* m_pInstance;
    static std::once_flag m_OnceFlag;
    
    ImageDecoderFactory();
    
    ~ImageDecoderFactory();
};

NAMESPACE_IMAGECODEC_END


#endif //RENDERENGINE_IMAGEDECODERFACTORY_H
