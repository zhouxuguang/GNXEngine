//
//  MetalView.mm
//  testNX
//
//  Created by zhouxuguang on 2022/9/3.
//

#import "MetalView.h"
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>
#include <TargetConditionals.h>
#include "RenderDelegate.h"
#include "WeakPtrProxy.h"

#if TARGET_OS_IPHONE

@interface MetalView ()
@property (nonatomic, strong) GNXRenderDelegate *renderDelegate;
@end

@implementation MetalView
{
    CADisplayLink* displayLink;
    NSThread *renderThread;
    CGSize viewSize;
    CAMetalLayer* metalLayer;
}

+ (Class)layerClass
{
    return [CAMetalLayer class];
}

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
    if ((self = [super initWithCoder:aDecoder]))
    {
    }
    
    return self;
}

- (instancetype)initWithFrame:(CGRect )frame
{
    if ((self = [super initWithFrame : frame]))
    {
        _renderDelegate = [GNXRenderDelegate new];
        float nativeScale = [UIScreen mainScreen].nativeScale;
        viewSize = CGSizeMake(self.frame.size.width * nativeScale, self.frame.size.height * nativeScale);
        metalLayer = (CAMetalLayer*)self.layer;
        
        metalLayer.device = MTLCreateSystemDefaultDevice();
        metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        metalLayer.contentsScale = nativeScale;
        metalLayer.framebufferOnly = YES;
        
        //1.创建线程
        renderThread = [[NSThread alloc]initWithTarget:[WeakPtrProxy proxyWithTarget:self] selector:@selector(RenderThread) object:nil];
        // 给线程设置名字
        [renderThread setName:@"MetalRenderThread"];
        [renderThread start];
    }
    
    return self;
}

-(void)RenderThread
{
    //1.获得子线程对应的runloop
    NSRunLoop *runLoop = [NSRunLoop currentRunLoop];
    
    //保证runloop不退出
    [runLoop addPort:[NSPort port] forMode:NSRunLoopCommonModes];
    
    
    int width = viewSize.width;
    int height = viewSize.height;
    
    [_renderDelegate initRenderWithHandle:metalLayer andType:RenderTypeMetal];
    [_renderDelegate resizeRender:width andHeight:height];
    
    displayLink = [CADisplayLink displayLinkWithTarget:_renderDelegate selector:@selector(drawFrame)];
    displayLink.paused = NO;
    [displayLink addToRunLoop:runLoop forMode:NSRunLoopCommonModes];
    
    //2.默认是没有开启 仅限子线程 主线程是没用的
    [runLoop run];
    
}

@end

#endif

#if TARGET_OS_MAC   //mac平台
//do something
#endif
