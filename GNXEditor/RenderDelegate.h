//
//  RenderDelegate.h
//  testNX
//
//  Created by zhouxuguang on 2022/9/3.
//

#import <Foundation/Foundation.h>
#import <QuartzCore/CALayer.h>

NS_ASSUME_NONNULL_BEGIN

typedef enum : NSUInteger
{
    RenderTypeAuto,    // 根据设备自动选择
    RenderTypeGLES,  // gles
    RenderTypeMetal,   // Metal
    RenderTypeVulkan,   // Vulkan
} RenderType;

@protocol RenderDelegate <NSObject>

-(void) initRenderWithHandle : (CALayer*) layer andType : (RenderType) renderType;

-(void) resizeRender : (NSUInteger) width andHeight : (NSUInteger) height;

-(void) drawFrame;

@end

@interface GNXRenderDelegate : NSObject <RenderDelegate>

@end

NS_ASSUME_NONNULL_END
