
#import "MetalView.h"
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "RenderDelegate.h"

float GetDevicePixelRatio()
{
    // 获取主显示器的屏幕 ID
    CGDirectDisplayID displayId = kCGDirectMainDisplay;
  
    // 获取设备像素比例
    CGDisplayModeRef mode = CGDisplayCopyDisplayMode(displayId);
    CGFloat pixelWidth = CGDisplayModeGetPixelWidth(mode);
    CGFloat width = CGDisplayModeGetWidth(mode);
  
    float devicePixelRatio = pixelWidth / width;
  
    CGDisplayModeRelease(mode);

    return devicePixelRatio;
}


@interface MetalView ()
@property (nonatomic, assign) BOOL isInited;
@property (nonatomic, assign) CVDisplayLinkRef displayLink;
@property (nonatomic, strong) GNXRenderDelegate *renderDelegate;
@end

@implementation MetalView

+ (Class)layerClass
{
    return [CAMetalLayer class];
}

- (CAMetalLayer*)metalLayer
{
    return (CAMetalLayer *)[self layer];
}

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
    if ((self = [super initWithCoder:aDecoder]))
    {
        self.layer = [[CAMetalLayer alloc] init];
        self.wantsLayer = YES;
        [self initrender];
        self.isInited = NO;
    }
    
    return self;
}

- (instancetype)initWithFrame:(CGRect )frame
{
    if ((self = [super initWithFrame : frame]))
    {
        //下面这两句代码是关键
        self.layer = [[CAMetalLayer alloc] init];
        self.wantsLayer = YES;
        [self initrender];
        self.isInited = NO;
        self.autoresizesSubviews = YES;
    }
    
    return self;
}

// Renderer output callback function
static CVReturn MyDisplayLinkCallback(
                                      CVDisplayLinkRef displayLink,
                                      const CVTimeStamp* now,
                                      const CVTimeStamp* outputTime,
                                      CVOptionFlags flagsIn,
                                      CVOptionFlags* flagsOut,
                                      void* displayLinkContext)
{
    CVReturn result = [(__bridge MetalView*)displayLinkContext getFrameForTime:outputTime];
    //CVReturn result;
    return result;
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldSize
{
    [super resizeSubviewsWithOldSize:oldSize];
    
    // 在这里执行需要的操作
    // 比如重新布局子视图或更新相关属性
    
    // 示例：打印新旧尺寸
    NSLog(@"旧尺寸：%f, %f", oldSize.width, oldSize.height);
    NSLog(@"新尺寸：%f, %f", self.frame.size.width, self.frame.size.height);
}

//
- (void)viewDidMoveToWindow
{
    CGRect screenBounds = self.frame;
    float scale = GetDevicePixelRatio();
    int viewWidth = ceilf(screenBounds.size.width * scale);
    int viewHeight = ceilf(screenBounds.size.height * scale);
    
    //[_renderDelegate resizeRender:viewWidth andHeight:viewHeight];
    printf("");
}

//- (void)viewDidMoveToSuperview
//{
//    CGRect screenBounds = self.frame;
//    printf("");
//}

- (BOOL)initrender
{
    CGRect screenBounds = self.frame;
//    float scale = self.window.screen.scale;
    float scale = 1.0;
    int viewWidth = ceilf(screenBounds.size.width * scale);
    int viewHeight = ceilf(screenBounds.size.height * scale);
    
    //CALayer* layer =  [self layer];
    
    _renderDelegate = [GNXRenderDelegate new];
    
    CAMetalLayer *metalLayer = [self metalLayer];
    metalLayer.device = MTLCreateSystemDefaultDevice();
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.contentsScale = scale;
    metalLayer.framebufferOnly = YES;
    
    CGSize drawSize = metalLayer.drawableSize;
    //[metalLayer setDrawableSize : CGSizeMake(viewWidth, viewHeight)];
    
#if 1
    
    // 创建_displayLink
    CVDisplayLinkCreateWithCGDisplay(CGMainDisplayID(), &_displayLink);

    CVDisplayLinkIsRunning(_displayLink);
    
    // 设置渲染输出的回调函数
    CVDisplayLinkSetOutputCallback(_displayLink, MyDisplayLinkCallback, (__bridge void *)self);

    // 启动 display link
    CVDisplayLinkStart(_displayLink);
    
#endif
    
    //[self getFrameForTime: nil];
    
    
    return YES;
}

- (CVReturn)getFrameForTime:(const CVTimeStamp*)outputTime
{
    /* We are on a secondary"CVDisplayLink" thread here. */
    
    CGRect screenBounds = self.frame;
    float scale = 1;
    int viewWidth = ceilf(screenBounds.size.width * scale);
    int viewHeight = ceilf(screenBounds.size.height * scale);
    
    if (0 == viewWidth || 0 == viewHeight)
    {
        return kCVReturnSuccess;
    }
    
    if (!self.isInited)
    {
        CAMetalLayer *metalLayer = [self metalLayer];
        metalLayer.device = MTLCreateSystemDefaultDevice();
        metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        metalLayer.contentsScale = scale;
        metalLayer.framebufferOnly = YES;
        
        [_renderDelegate initRenderWithHandle:metalLayer andType:RenderTypeMetal];
        [_renderDelegate resizeRender:viewWidth andHeight:viewHeight];
        
        self.isInited = YES;
    }

    
    [self.renderDelegate drawFrame];
    //[self draw];

    return kCVReturnSuccess;
}

- (void)draw
{
    CAMetalLayer *metalLayer = (CAMetalLayer *)[self layer];
    
    CGSize drawSize = metalLayer.drawableSize;
    
    id<CAMetalDrawable> drawable =  [metalLayer nextDrawable];
    id<MTLTexture> texture = drawable.texture;

    MTLRenderPassDescriptor *passDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    passDescriptor.colorAttachments[0].texture = texture;
    passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(1, 0, 0, 1);

    id<MTLCommandQueue> commandQueue = [metalLayer.device newCommandQueue];

    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];

    id <MTLRenderCommandEncoder> commandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
    [commandEncoder endEncoding];

    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
}

- (void)dealloc
{
    //[super dealloc];
    CVDisplayLinkStop(_displayLink);
}

@end
