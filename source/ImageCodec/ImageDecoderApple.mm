//
//  image_decoder_apple.mm
//  
//
//  Created by zhouxuguang on 2019/7/23.
//  Copyright © 2019 zhouxuguang. All rights reserved.
//

#include "ImageDecoderApple.h"
#include "ImageDecoderImpl.h"
#import <Foundation/Foundation.h>
#if (TARGET_OS_MAC & !TARGET_OS_SIMULATOR)
    #import <AppKit/AppKit.h>
#elif TARGET_OS_IOS
    #import <UIKit/UIKit.h>
#endif
#include <Accelerate/Accelerate.h>

NAMESPACE_IMAGECODEC_BEGIN

void DestroyCFDataRef(void* pData)
{
    if (pData)
    {
        CFDataRef rawData = (CFDataRef)pData;
        CFRelease(rawData);
    }
}

unsigned char* GetCFDataRef(void* pData)
{
    if (pData)
    {
        CFDataRef rawData = (CFDataRef)pData;
        return (unsigned char*)CFDataGetBytePtr(rawData);
    }
    
    return NULL;
}

//static CGImageRef FlipImage(CGImageRef image)
//{
//    if (!image)
//    {
//        return NULL;
//    }
//    
//    size_t width = CGImageGetWidth(image);
//    size_t height = CGImageGetHeight(image);
//    CGContextRef context = CGBitmapContextCreate(NULL, width, height,
//                                             CGImageGetBitsPerComponent(image),
//                                             CGImageGetBytesPerRow(image),
//                                             CGImageGetColorSpace(image),
//                                             CGImageGetBitmapInfo(image));
//    
//    if (!context)
//    {
//        return NULL;
//    }
//    
//    CGAffineTransform transform = CGAffineTransformMake(1, 0, 0, -1, 0, height);
//    CGContextConcatCTM(context, transform);
//    CGContextDrawImage(context, CGRectMake(0, 0, width, height), image);
//    
//    CGImageRef cgImage = CGBitmapContextCreateImage(context);
//    if (!cgImage)
//    {
//        CGContextRelease(context);
//        return NULL;
//    }
//    CGContextRelease(context);
//    
//    return cgImage;
//}

static int getChannelCount(ImagePixelFormat pixelFormat)
{
    switch (pixelFormat)
    {
        case FORMAT_GRAY8:
            return 1;
            break;
            
        case FORMAT_GRAY8_ALPHA8:
            return 2;
            break;
            
        case FORMAT_RGBA8:
            return 4;
            break;
            
        case FORMAT_RGB8:
            return 3;
            break;
            
        default:
            break;
    }
    
    return 0;
}

bool DecodeAppleImage(const void *buffer, size_t size, VImage *bitmap)
{
    if (NULL == bitmap)
    {
        return false;
    }
    
    uint32_t nWidth = 0;
    uint32_t nHeight = 0;
    ImagePixelFormat pixelFormat = FORMAT_UNKNOWN;
    bool bPremultipliedAlpha = bitmap->HasPremultipliedAlpha();
    
    uint8_t* pData = DecodeImageData_APPLE((const uint8_t*)buffer, size, &nWidth, &nHeight, pixelFormat, bPremultipliedAlpha);
    if (!pData)
    {
        return false;
    }
    
    bool hasAlpha = hasAlphaChannel(pixelFormat);
    if (hasAlpha && !bPremultipliedAlpha)
    {
        PremultipliedAlpha(pData, nWidth, nHeight, getChannelCount(pixelFormat));
    }
    
    bitmap->SetImageInfo(pixelFormat, nWidth, nHeight, pData, free);
    return true;
}

static void ParserImageFormat(CGImageRef imageRef, CGColorSpaceRef colorSpace, ImagePixelFormat &pixelFormat, bool& bPreMultyAlpha)
{
    bPreMultyAlpha = false;
    CGImageAlphaInfo alphaInfo = CGImageGetAlphaInfo(imageRef);
    if (alphaInfo == kCGImageAlphaPremultipliedLast || alphaInfo == kCGImageAlphaPremultipliedFirst)
    {
        bPreMultyAlpha = true;
    }
    BOOL hasAlpha = alphaInfo != kCGImageAlphaNone;
    
    switch (CGColorSpaceGetModel(colorSpace))
    {
        case kCGColorSpaceModelMonochrome:
            pixelFormat = hasAlpha ? FORMAT_GRAY8_ALPHA8 : FORMAT_GRAY8;
            break;
            
        case kCGColorSpaceModelRGB:
        {
            CGColorSpaceRef dstSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
            CGColorSpaceRef srcSpace = CGImageGetColorSpace(imageRef);

            // 是sRGB颜色空间
            if (CFEqual(srcSpace, dstSpace))
            {
                CGColorSpaceRelease(dstSpace);
                pixelFormat = hasAlpha ? FORMAT_SRGB8_ALPHA8 : FORMAT_SRGB8;
                break;
            }
            
            pixelFormat = hasAlpha ? FORMAT_RGBA8 : FORMAT_RGB8;
            
            break;
        }
            
        default:
            break;
    }
}

uint8_t* DecodeImageData_APPLE(const uint8_t* pImageData,
                             size_t dataLen,
                             uint32_t* uiWidth,
                             uint32_t* uiHeight,
                             ImagePixelFormat &pixelFormat,
                             bool& bPreMultyAlpha)
{
    uint8_t *pData = NULL;
    @autoreleasepool {
        
        NSData *data = [NSData dataWithBytesNoCopy: (void *)pImageData length: dataLen freeWhenDone: NO];
        
#if TARGET_OS_MAC
        CGImageSourceRef imageSource = CGImageSourceCreateWithData((CFDataRef)data, NULL);
        CGImageRef imageRef = CGImageSourceCreateImageAtIndex(imageSource, 0, NULL);
#elif TARGET_OS_IOS
        UIImage *image = [UIImage imageWithData: data];
        if (!image)
        {
            return NULL;
        }
        
        CGImageRef imageRef = image.CGImage;
#endif
        
        if (!imageRef)
        {
            return NULL;
        }
        
        *uiWidth = (uint32_t)CGImageGetWidth(imageRef);
        *uiHeight = (uint32_t)CGImageGetHeight(imageRef);
        
        CGColorSpaceRef colorSpace = CGImageGetColorSpace(imageRef);
        CGColorSpaceModel colorModel = CGColorSpaceGetModel(colorSpace);
        
        // 图片的ColorSpaceModel为kCGColorSpaceModelUnknown，kCGColorSpaceModelCMYK，和kCGColorSpaceModelIndexed时，说明该ColorSpace不支持，不能直接创建纹理
        BOOL unSupportedColorSpace = (colorModel == kCGColorSpaceModelUnknown ||
                                      colorModel == kCGColorSpaceModelCMYK ||
                                      colorModel == kCGColorSpaceModelIndexed);
        
        //是否重新生成RGB的图像
        BOOL geneRGBImage = TRUE;
        
        //处理16位的图片，渲染的时候不支持，必须转换为8位的
        size_t nBitsPerComponent = CGImageGetBitsPerComponent(imageRef);
        if (nBitsPerComponent > 8) {
            unSupportedColorSpace = YES;
        }
        if (colorModel == kCGColorSpaceModelMonochrome) {
            geneRGBImage = NO;
            
            // 如果是带alpha的图像，则生成RGBA的图像
            CGImageAlphaInfo alphaInfo = CGImageGetAlphaInfo(imageRef);
            BOOL hasAlpha = alphaInfo != kCGImageAlphaNone;
            
            if (hasAlpha) {
                geneRGBImage = YES;
            }
        }
       
        if (unSupportedColorSpace) {
            // 不支持ColorSpace，ColorSpace使用RGB或者灰度模式
            size_t nLineBytes = 4 * (*uiWidth);
            uint32_t bitmapInfo = kCGBitmapByteOrderDefault|kCGImageAlphaPremultipliedLast;
            if (geneRGBImage)
            {
                colorSpace = CGColorSpaceCreateDeviceRGB();
            }
            else
            {
                colorSpace = CGColorSpaceCreateDeviceGray();
                nLineBytes = *uiWidth;
                bitmapInfo = kCGImageAlphaOnly;
            }
            
            size_t dataLength = nLineBytes * *uiHeight;
            pData = (uint8_t*)malloc(dataLength);
            if (nullptr == pData) {
                CGColorSpaceRelease(colorSpace);
                return nullptr;
            }
            
            // kCGImageAlphaNone is not supported in CGBitmapContextCreate
            CGContextRef context = CGBitmapContextCreate(pData,
                                                         *uiWidth,
                                                         *uiHeight,
                                                         8,
                                                         nLineBytes,
                                                         colorSpace,
                                                         bitmapInfo);
            
            //生成RGBA格式的图片
            CGContextDrawImage(context, CGRectMake(0, 0, *uiWidth, *uiHeight), imageRef);
            imageRef = CGBitmapContextCreateImage(context);
            
            ParserImageFormat(imageRef, colorSpace, pixelFormat, bPreMultyAlpha);
            
            // 释放资源
            CGContextRelease(context);
            CGImageRelease(imageRef);
            CGColorSpaceRelease(colorSpace);
            
        } else {
            if (@available(iOS 7.0, *)) {

                //这里可能做了字节对齐，所以用原始的字节数
                size_t nLineBytes = CGImageGetBitsPerPixel(imageRef) / 8;
                nLineBytes *= (*uiWidth);
                pData = (uint8_t*)malloc(nLineBytes * *uiHeight);
                if (nullptr == pData) {
                    return nullptr;
                }

                vImage_Buffer imageBuffer;
                imageBuffer.rowBytes = nLineBytes;
                imageBuffer.data = (void*)pData;
                vImage_CGImageFormat format;
                format.bitmapInfo = CGImageGetBitmapInfo(imageRef);
                format.decode = CGImageGetDecode(imageRef);
                format.version = 0;
                format.bitsPerComponent = (uint32_t)CGImageGetBitsPerComponent(imageRef);
                format.bitsPerPixel = (uint32_t)CGImageGetBitsPerPixel(imageRef);
                format.colorSpace = colorSpace;
                format.renderingIntent = CGImageGetRenderingIntent(imageRef);
                vImage_Error error = vImageBuffer_InitWithCGImage(&imageBuffer, &format, nullptr, imageRef, kvImageNoAllocate);
                if (error != kvImageNoError) {
                    free(pData);
                    pData = NULL;
                }
            } else {
                CFDataRef rawData = CGDataProviderCopyData(CGImageGetDataProvider(imageRef));
                size_t dataLength = CFDataGetLength(rawData);
                pData = (uint8_t*)malloc(dataLength);
                if (nullptr == pData) {
                    CFRelease(rawData);
                    return nullptr;
                }
                memcpy(pData, CFDataGetBytePtr(rawData), dataLength);
                CFRelease(rawData);
            }
            
            ParserImageFormat(imageRef, colorSpace, pixelFormat, bPreMultyAlpha);
        }
    }
    
    return pData;
}

NAMESPACE_IMAGECODEC_END
