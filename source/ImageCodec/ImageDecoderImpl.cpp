//
// Created by Zhou,Xuguang on 2015/10/27.
//

#include "ImageDecoderImpl.h"
#include "ImageDecoderFactory.h"

//#ifdef __ANDROID__
//#include "android_fopen.h"
//#endif

#include <stdio.h>
#include <memory.h>
#include <math.h>

NAMESPACE_IMAGECODEC_BEGIN

bool hasAlphaChannel(ImagePixelFormat type)
{
    switch (type)
    {
        case FORMAT_GRAY8_ALPHA8:
        case FORMAT_RGBA8:
            return true;
            break;
            
        default:
            break;
    }
    
    return false;
}

//8位颜色值到float颜色值的映射表
static const float floatColorTable[256] = {0.000000, 0.003922, 0.007843, 0.011765, 0.015686, 0.019608, 0.023529, 0.027451, 0.031373, 0.035294, 0.039216, 0.043137, 0.047059, 0.050980, 0.054902, 0.058824,
    0.062745, 0.066667, 0.070588, 0.074510, 0.078431, 0.082353, 0.086275, 0.090196, 0.094118, 0.098039, 0.101961, 0.105882, 0.109804, 0.113725, 0.117647, 0.121569, 0.125490, 0.129412, 0.133333, 0.137255,
    0.141176, 0.145098, 0.149020, 0.152941, 0.156863, 0.160784, 0.164706, 0.168627, 0.172549, 0.176471, 0.180392, 0.184314, 0.188235, 0.192157, 0.196078, 0.200000, 0.203922, 0.207843, 0.211765, 0.215686,
    0.219608, 0.223529, 0.227451, 0.231373, 0.235294, 0.239216, 0.243137, 0.247059, 0.250980, 0.254902, 0.258824, 0.262745, 0.266667, 0.270588, 0.274510, 0.278431, 0.282353, 0.286275, 0.290196, 0.294118,
    0.298039, 0.301961, 0.305882, 0.309804, 0.313726, 0.317647, 0.321569, 0.325490, 0.329412, 0.333333, 0.337255, 0.341176, 0.345098, 0.349020, 0.352941, 0.356863, 0.360784, 0.364706, 0.368627, 0.372549,
    0.376471, 0.380392, 0.384314, 0.388235, 0.392157, 0.396078, 0.400000, 0.403922, 0.407843, 0.411765, 0.415686, 0.419608, 0.423529, 0.427451, 0.431373, 0.435294, 0.439216, 0.443137, 0.447059, 0.450980,
    0.454902, 0.458824, 0.462745, 0.466667, 0.470588, 0.474510, 0.478431, 0.482353, 0.486275, 0.490196, 0.494118, 0.498039, 0.501961, 0.505882, 0.509804, 0.513726, 0.517647, 0.521569, 0.525490, 0.529412,
    0.533333, 0.537255, 0.541176, 0.545098, 0.549020, 0.552941, 0.556863, 0.560784, 0.564706, 0.568627, 0.572549, 0.576471, 0.580392, 0.584314, 0.588235, 0.592157, 0.596078, 0.600000, 0.603922, 0.607843,
    0.611765, 0.615686, 0.619608, 0.623529, 0.627451, 0.631373, 0.635294, 0.639216, 0.643137, 0.647059, 0.650980, 0.654902, 0.658824, 0.662745, 0.666667, 0.670588, 0.674510, 0.678431, 0.682353, 0.686275,
    0.690196, 0.694118, 0.698039, 0.701961, 0.705882, 0.709804, 0.713726, 0.717647, 0.721569, 0.725490, 0.729412, 0.733333, 0.737255, 0.741176, 0.745098, 0.749020, 0.752941, 0.756863, 0.760784, 0.764706,
    0.768628, 0.772549, 0.776471, 0.780392, 0.784314, 0.788235, 0.792157, 0.796079, 0.800000, 0.803922, 0.807843, 0.811765, 0.815686, 0.819608, 0.823529, 0.827451, 0.831373, 0.835294, 0.839216, 0.843137,
    0.847059, 0.850980, 0.854902, 0.858824, 0.862745, 0.866667, 0.870588, 0.874510, 0.878431, 0.882353, 0.886275, 0.890196, 0.894118, 0.898039, 0.901961, 0.905882, 0.909804, 0.913726, 0.917647, 0.921569,
    0.925490, 0.929412, 0.933333, 0.937255, 0.941177, 0.945098, 0.949020, 0.952941, 0.956863, 0.960784, 0.964706, 0.968628, 0.972549, 0.976471, 0.980392, 0.984314, 0.988235, 0.992157, 0.996078, 1.000000};

void PremultipliedAlpha(unsigned char* pImateData, int width, int height, int bytesPerComponent)
{
    if (!pImateData)
    {
        return;
    }
    
    //PremultipliedAlpha
    if (bytesPerComponent == 4)
    {
        int nOffset = 0;
        for (int nRows = 0; nRows < height; nRows ++)
        {
            for (int nCols = 0; nCols < width; nCols ++)
            {
                uint8_t bAlpha = pImateData[nOffset + 3];
                
                *(pImateData + nOffset) = round(pImateData[nOffset] * floatColorTable[bAlpha]);
                *(pImateData + nOffset + 1) = round(pImateData[nOffset + 1] * floatColorTable[bAlpha]);
                *(pImateData + nOffset + 2) = round(pImateData[nOffset + 2] * floatColorTable[bAlpha]);
                
                nOffset += 4;
            }
        }
    }
    
    else if (bytesPerComponent == 2)
    {
        int nOffset = 0;
        for (int nRows = 0; nRows < height; nRows ++)
        {
            for (int nCols = 0; nCols < width; nCols ++)
            {
                uint8_t bAlpha = pImateData[nOffset + 1];
                
                *(pImateData + nOffset) = round(pImateData[nOffset] * floatColorTable[bAlpha]);
                
                nOffset += 2;
            }
        }
    }
}

bool ImageDecoderImpl::DecodeFile(const char *fileName, VImage *bitmap, ImageStoreFormat *format)
{
    if (NULL == fileName || NULL == bitmap)
    {
        return false;
    }
    
    //后续Android  asst目录又得考虑

//#ifdef __ANDROID__
//
//    FILE *fp = NULL;
//    const char* pTmp = NULL;
//
//    //判断路径名是否以assets/开头
//    if ((pTmp = strstr(fileName, "assets/")) != NULL)
//    {
//        pTmp += 7;
//        fp = android_fopen(pTmp, "rb");
//    }
//    else
//    {
//        fp = fopen(fileName, "rb");
//    }
//
//#else
//    FILE *fp = fopen(fileName, "rb");
//#endif

    FILE *fp = fopen(fileName, "rb");

    if (NULL == fp)
    {
        return false;
    }

    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    unsigned char *pBuf = new unsigned char[size];

    fread(pBuf, size, 1, fp);
    fclose(fp);

    bool bRet = ImageDecoderImpl::DecodeMemory(pBuf, size, bitmap, format);
    delete[]pBuf;
    return bRet;

}

bool ImageDecoderImpl::DecodeMemory(const void *buffer, size_t size, VImage *bitmap, ImageStoreFormat *format)
{
    if (NULL == buffer || 0 == size || NULL == bitmap)
    {
        return false;
    }

    ImageDecoderFactory* pInstance = ImageDecoderFactory::GetInstance();
    ImageDecoderImplPtr pDecoder = pInstance->GetImageDecoder(buffer, size);
    if (NULL == pDecoder)
    {
        return false;
    }

    bool bRet = pDecoder->onDecode(buffer, size, bitmap);
    if (format)
    {
        *format = pDecoder->GetFormat();
    }

    return bRet;
}

ImageStoreFormat ImageDecoderImpl::GetFormat() const
{
    return kUnknown_Format;
}

NAMESPACE_IMAGECODEC_END
