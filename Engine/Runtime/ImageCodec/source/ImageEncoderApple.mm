//
//  image_encoder_apple.mm
//  
//
//  Created by Zhou,Xuguang on 2019/10/18.
//  Copyright © 2019年 zhouxuguang. All rights reserved.
//

#include "ImageEncoderApple.h"
#include <Foundation/Foundation.h>
#include <CoreServices/CoreServices.h>

NAMESPACE_IMAGECODEC_BEGIN

CGImageRef imageFormVImage(const VImage& image)
{
    @autoreleasepool
    {
        unsigned int width = image.GetWidth();
        unsigned int height = image.GetHeight();
        const size_t bytesPerRow = image.GetBytesPerRow();
        
        size_t bitsPerComponent = 0;
        size_t bitsPerPixel = 0;
        CGColorSpaceRef colorSpace = NULL;
        CGBitmapInfo bitmapInfo = kCGImageAlphaNone;
        
        const ImagePixelFormat format = image.GetFormat();
        
        switch (format)
        {
            case FORMAT_RGB8:
                bitsPerComponent = 8;
                bitsPerPixel = 24;
                colorSpace = CGColorSpaceCreateDeviceRGB();
                bitmapInfo = kCGImageAlphaNone;
                break;
                
            case FORMAT_RGBA8:
                bitsPerComponent = 8;
                bitsPerPixel = 32;
                colorSpace = CGColorSpaceCreateDeviceRGB();
                bitmapInfo = kCGImageAlphaPremultipliedLast | kCGBitmapByteOrderDefault;
                break;
                
            case FORMAT_R5G6B5:
                bitsPerComponent = 5;
                bitsPerPixel = 16;
                colorSpace = CGColorSpaceCreateDeviceRGB();
                bitmapInfo = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrderDefault;
                break;
                
            case FORMAT_RGBA4444:
                bitsPerComponent = 4;
                bitsPerPixel = 16;
                colorSpace = CGColorSpaceCreateDeviceRGB();
                bitmapInfo = kCGImageAlphaPremultipliedLast | kCGBitmapByteOrderDefault;
                break;
                
            case FORMAT_RGB5A1:
                bitsPerComponent = 5;
                bitsPerPixel = 16;
                colorSpace = CGColorSpaceCreateDeviceRGB();
                bitmapInfo = kCGImageAlphaPremultipliedLast | kCGBitmapByteOrderDefault;
                break;
                
            case FORMAT_GRAY8:
                bitsPerComponent = 8;
                bitsPerPixel = 8;
                colorSpace = CGColorSpaceCreateDeviceGray();
                bitmapInfo = kCGImageAlphaOnly;
                break;
                
            case FORMAT_GRAY8_ALPHA8:
                bitsPerComponent = 8;
                bitsPerPixel = 16;
                colorSpace = CGColorSpaceCreateDeviceGray();
                bitmapInfo = kCGImageAlphaPremultipliedLast | kCGBitmapByteOrderDefault;
                break;
                
            default:
                return NULL;
        }
        
        const size_t bufferLength = bytesPerRow * height;
        NSData *data = [NSData dataWithBytesNoCopy : image.GetPixels() length : bufferLength freeWhenDone : NO];
        CGDataProviderRef provider = CGDataProviderCreateWithCFData((__bridge CFDataRef)data);
        
        // Creating CGImage from image buffer
        CGImageRef imageRef = CGImageCreate(width,
                                            height,
                                            bitsPerComponent,
                                            bitsPerPixel,
                                            bytesPerRow,
                                            colorSpace,
                                            bitmapInfo,
                                            provider,
                                            NULL,
                                            false,
                                            kCGRenderingIntentDefault
                                            );
        
        //释放相关资源
        CGDataProviderRelease(provider);
        CGColorSpaceRelease(colorSpace);
        
        return imageRef;
    }
}

bool saveCGImageToFile(const char* fileName, CGImageRef imageRef, ImageStoreFormat format, int quality)
{
    if (!fileName)
    {
        return false;
    }
    
    @autoreleasepool
    {
        if (!imageRef)
        {
            return false;
        }
        
        NSString *strPath = [NSString stringWithUTF8String:fileName];
        CFURLRef url = (__bridge CFURLRef)[NSURL fileURLWithPath:strPath];
        
        CGImageDestinationRef destination = NULL;
        bool isJpeg = false;
        switch (format)
        {
            case kPNG_Format:
                destination = CGImageDestinationCreateWithURL(url, kUTTypePNG, 1, NULL);
                break;
                
            case kJPEG_Format:
                destination = CGImageDestinationCreateWithURL(url, kUTTypeJPEG, 1, NULL);
                isJpeg = true;
                break;
                
            case kBMP_Format:
                destination = CGImageDestinationCreateWithURL(url, kUTTypeBMP, 1, NULL);
                break;
                
            default:
                CGImageRelease(imageRef);
                return false;
        }
        
        NSDictionary *dict = nil;
        if (isJpeg)
        {
            NSNumber *nsQuality = [NSNumber numberWithFloat: quality * 0.01];
            dict = [NSDictionary dictionaryWithObject:nsQuality forKey: (__bridge NSString*)kCGImageDestinationLossyCompressionQuality];
        }
        
        //将图像写入到newImageData中
        CGImageDestinationAddImage(destination, imageRef, (__bridge CFDictionaryRef)dict);
        if (!CGImageDestinationFinalize(destination))
        {
            CFRelease(destination);
            return false;
        }
        
        CFRelease(destination);
        
        return true;
    }
}

bool saveCGImageToBuffer(std::vector<unsigned char>& dataStream, CGImageRef imageRef, ImageStoreFormat format, int quality)
{
    @autoreleasepool
    {
        if (!imageRef)
        {
            return false;
        }
        
        CFMutableDataRef newImageData = CFDataCreateMutable(NULL, 0);
        CGImageDestinationRef destination = NULL;
        bool isJpeg = false;
        switch (format)
        {
            case kPNG_Format:
                destination = CGImageDestinationCreateWithData(newImageData, kUTTypePNG, 1, NULL);
                break;
                
            case kJPEG_Format:
                destination = CGImageDestinationCreateWithData(newImageData, kUTTypeJPEG, 1, NULL);
                isJpeg = true;
                break;
                
            case kBMP_Format:
                destination = CGImageDestinationCreateWithData(newImageData, kUTTypeBMP, 1, NULL);
                break;
                
            default:
                CFRelease(newImageData);
                CGImageRelease(imageRef);
                return false;
        }
        
        NSDictionary *dict = nil;
        if (isJpeg)
        {
            NSNumber *nsQuality = [NSNumber numberWithFloat: quality * 0.01];
            dict = [NSDictionary dictionaryWithObject:nsQuality forKey: (__bridge NSString*)kCGImageDestinationLossyCompressionQuality];
        }
        
        //将图像写入到newImageData中
        CGImageDestinationAddImage(destination, imageRef, (__bridge CFDictionaryRef)dict);
        if (!CGImageDestinationFinalize(destination))
        {
            CFRelease(destination);
            return false;
        }
        NSData *newImage = CFBridgingRelease(newImageData);
        dataStream.resize([newImage length]);
        memcpy(dataStream.data(), [newImage bytes], [newImage length]);
        
        CFRelease(destination);
        
        return true;
    }
}

NAMESPACE_IMAGECODEC_END
