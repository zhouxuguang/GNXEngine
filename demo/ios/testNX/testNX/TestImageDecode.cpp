//
//  TestImageDecode.cpp
//  testNX
//
//  Created by zhouxuguang on 2021/5/13.
//

#include "TestImageDecode.h"
#include "ResourceUtil.h"

VImagePtr getImage()
{
    std::string strPath = getTexturePath();
    VImagePtr image = std::make_shared<VImage>();
    ImageDecoder::DecodeFile(strPath.c_str(), image.get());
    return image;
}
