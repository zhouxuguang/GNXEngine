
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
    FORMAT_UNKNOWN = -1,
    FORMAT_GRAY8 = 0,            //gray
    FORMAT_GRAY8_ALPHA8 = 1,     //GRAY AND ALPHA
    FORMAT_RGBA8 = 2,            //RGBA32位
    FORMAT_RGB8 = 3,             //RGB
    FORMAT_RGBA4444 = 4,
    FORMAT_RGB5A1 = 5,
    FORMAT_R5G6B5 = 6,
    
    FORMAT_SRGB8_ALPHA8 = 7,             //sRGB_alpha
    FORMAT_SRGB8 = 8,             //sRGB

	FORMAT_RGBA32Float = 9,            //RGBA32位float
	FORMAT_RGB32Float = 10,             //RGB float
};

//图像的存储格式  bmp/png等
enum ImageStoreFormat
{
    kUnknown_Format,
    kBMP_Format,
    kJPEG_Format,
    kPNG_Format,
    kTGA_Format,
    kWEBP_Format,
    kHDR_Format
};

/**
    图像数据的类，表示图像的基本信息，以及图像数据
    通过imageDecoder解析出来的图像数据，左上角的纹理坐标是(0,0)，左下角是(0,1)
 **/
class IMAGECODEC_API VImage
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

    uint32_t GetBytesPerPixels() const;
    
    bool HasPremultipliedAlpha() const;
    
    void SetPremultipliedAlpha(bool bPremultipliedAlpha);
    
    void SetMipCount(int mipCount);
    
    int GetMipCount() const;
    
    int GetImageSize(int level = 0) const;
    
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
    uint32_t                    mWidth;                //宽
    uint32_t                    mHeight;               //高
    uint32_t                    mBytesPerRow;          //每个像素的字节数，根据m_eFormat决定
    uint32_t                    mBytesPerPixels;       //每个像素多少个字节
    uint16_t                    mMipCount;             //mipcount
    ImagePixelFormat            mFormat;               //数据格式
    bool                        mPremultipliedAlpha;   //是否预先乘以alpha
    void *                      mData;                 //数据
    DeleteFun                   mDeleteFunc;           //删除回调函数
};

typedef std::shared_ptr<VImage> VImagePtr;

NAMESPACE_IMAGECODEC_END


#endif //RENDERENGINE_IMAGE_H
