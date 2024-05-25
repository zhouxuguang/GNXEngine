//
//  EAGLView.m
//  testNX
//
//  Created by zhouxuguang on 2021/5/3.
//

#import "EAGLView.h"
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#include "RenderDelegate.h"
#include "WeakPtrProxy.h"

@interface EAGLView()
@property (nonatomic, strong) GNXRenderDelegate *renderDelegate;
@end

@implementation EAGLView
{
    CADisplayLink* displayLink;
    NSThread *renderThread;
    CGSize viewSize;
    CALayer* glLayer;
}

// You must implement this method
+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

// When created via code however, we get initWithFrame
-(id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if(self != nil)
    {
        _renderDelegate = [GNXRenderDelegate new];
        viewSize = self.frame.size;
        glLayer = self.layer;
        
        //1.创建线程
        renderThread = [[NSThread alloc]initWithTarget:[WeakPtrProxy proxyWithTarget:self] selector:@selector(RenderThread) object:nil];
        // 给线程设置名字
        [renderThread setName:@"GLRenderThread"];
        [renderThread start];
    }
    return self;
}

//The EAGL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:.
- (id)initWithCoder:(NSCoder*)coder
{
    self = [super initWithCoder:coder];
    if (self)
    {
    }
    
    return self;
}


-(void)RenderThread
{
    //1.获得子线程对应的runloop
    NSRunLoop *runLoop = [NSRunLoop currentRunLoop];
    
    //保证runloop不退出
    [runLoop addPort:[NSPort port] forMode:NSRunLoopCommonModes];
    
    float nativeScale = [UIScreen mainScreen].nativeScale;
    int width = viewSize.width * nativeScale;
    int height = viewSize.height * nativeScale;
    
    glLayer.contentsScale = nativeScale;
    
    [_renderDelegate initRenderWithHandle:glLayer andType:RenderTypeGLES];
    [_renderDelegate resizeRender:width andHeight:height];
    
    displayLink = [CADisplayLink displayLinkWithTarget:_renderDelegate selector:@selector(drawFrame)];
    displayLink.paused = NO;
    [displayLink addToRunLoop:runLoop forMode:NSRunLoopCommonModes];
    
    //2.默认是没有开启 仅限子线程 主线程是没用的
    [runLoop run];
    
}

@end
