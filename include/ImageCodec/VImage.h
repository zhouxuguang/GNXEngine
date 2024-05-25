
//
// Created by Zhou,Xuguang on 2015/10/22.
//

#ifndef RENDERENGINE_IMAGE_H
#define RENDERENGINE_IMAGE_H

#include "Define.h"
#include <memory>

NAMESPACE_IMAGECODEC_BEGIN

//图像的像素格式
typedef uint32_t ImagePixelFormat;
enum
{
    FORMAT_UNKNOWN,
    FORMAT_GRAY8,            //gray
    FORMAT_GRAY8_ALPHA8,     //GRAY AND ALPHA
    FORMAT_RGBA8,            //RGBA32位
    FORMAT_RGB8,             //RGB
    FORMAT_RGBA4444,
    FORMAT_RGB5A1,
    FORMAT_R5G6B5,
    
    FORMAT_SRGB8_ALPHA8,             //sRGB_alpha
    FORMAT_SRGB8,             //sRGB
    
    //压缩纹理
    // EAC and ETC2 compressed formats, mandated by OpenGL ES 3.0
    FORMAT_EAC_R = 41,
    FORMAT_EAC_R_SIGNED = 42,
    FORMAT_EAC_RG = 43,
    FORMAT_EAC_RG_SIGNED = 44,
    FORMAT_ETC2_RGB = 45,
    FORMAT_ETC2_SRGB = 46,
    FORMAT_ETC2_RGBA1 = 47,
    FORMAT_ETC2_SRGBA1 = 48,
    FORMAT_ETC2_RGBA8 = 49,
    FORMAT_ETC2_SRGBA8 = 50,
    
    FORMAT_ETC1_RGB = 51,
};

inline bool IsCompressedFormat(ImagePixelFormat format)
{
    return format >= FORMAT_EAC_R;
}

//图像的存储格式  bmp/png等
enum ImageStoreFormat
{
    kUnknown_Format,
    kBMP_Format,
    kJPEG_Format,
    kPNG_Format,
    kTGA_Format,
    kWEBP_Format,
};

struct MipDataSizeAndOffset
{
    int dataSize;
    int offset;
    uint16_t levelWidth;
    uint16_t levelHeight;
    uint32_t bytesPerRow;
};

/**
    图像数据的类，支持普通纹理和压缩纹理
    通过imageDecoder解析出来的图像数据，左上角的纹理坐标是(0,0)，左下角是(0,1)
 **/
class VImage
{
public:
    VImage();

    VImage(ImagePixelFormat format,
           uint32_t nWidth,
           uint32_t nHeight,
          const void* pData);

    ~VImage();
    
    /**
    * 分配图像内存
    *
    * @return 无
    */
    void AllocPixels();

    /**
            获得图像数据
     */
    uint8_t* GetPixels(int level = 0) const;

    ImagePixelFormat GetFormat() const;

    uint32_t GetWidth(int level = 0) const;

    uint32_t GetHeight(int level = 0) const;

    uint32_t GetBytesPerRow() const;
    
    bool HasPremultipliedAlpha() const;
    
    void SetPremultipliedAlpha(bool bPremultipliedAlpha);
    
    void SetMipCount(int mipCount);
    
    int GetMipCount() const;
    
    int GetImageSize(int level = 0) const;
    
    void SetMipDataSizeAndOffset(const std::vector<MipDataSizeAndOffset>& dataSizeAndOffset);
    
    /**
    * 设置图像的基本信息，基本信息设置后再使用的话需要调用AllocPixels分配内存
    *
    * @param format 像素格式
    * @param nWidth 图像宽度
    * @param nHeight 图像高度
    * @return 无
    */
    void SetImageInfo(ImagePixelFormat format, uint32_t nWidth, uint32_t nHeight);
    
    /**
    * 删除图像内存的回调函数
    *
    * @return 无
    */
    typedef void(*DeleteFun)(void *);
    
    /**
    * 设置图像的基本信息
    *
    * @param format 像素格式
    * @param nWidth 图像宽度
    * @param nHeight 图像高度
    * @param pData 图像数据
    * @param pDeleteFunc 如果图像数据的内存释放需要由Image内部来释放，那么传入删除像素的回调函数，不需要的话传入NULL
    * @return 无
    */
    void SetImageInfo(ImagePixelFormat format, uint32_t nWidth, uint32_t nHeight, const void* pData, DeleteFun pDeleteFunc);
    
    /**
    * 释放图像的内存数据
    *
    * @return 无
    */
    void Release();

private:
    uint32_t                    m_nWidth;                //宽
    uint32_t                    m_nHeight;               //高
    uint32_t                    m_nEncodedWidth;        //压缩纹理编码后的宽
    uint32_t                    m_nEncodedHeight;       //压缩纹理编码后的高
    uint32_t                    m_nBytesPerRow;        //每个像素的字节数，根据m_eFormat决定
    uint16_t                   m_mipCount;             //mipcount
    ImagePixelFormat            m_eFormat;               //数据格式
    bool                        m_bPremultipliedAlpha;   //是否预先乘以alpha
    void *                      m_pData;                 //数据
    DeleteFun                   m_pDeleteFunc;           //删除回调函数
    
    std::vector<MipDataSizeAndOffset> m_dataSizeAndOffsets;
};

typedef std::shared_ptr<VImage> VImagePtr;

NAMESPACE_IMAGECODEC_END


#endif //RENDERENGINE_IMAGE_H
