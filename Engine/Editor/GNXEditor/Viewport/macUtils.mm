//
//  macUtils.mm
//  GNXEditor
//
//  Created by zhouxuguang on 2026/1/24.
//

#include "macUtils.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

void* GetMetalLayer(WId wid)
{
    const id<MTLDevice> gpu = MTLCreateSystemDefaultDevice();
    CAMetalLayer *metalLayer = [CAMetalLayer layer];
    metalLayer.device = gpu;
    metalLayer.opaque = YES;
    metalLayer.contentsScale = 1.0;
    metalLayer.framebufferOnly = YES;

    NSView* nsView = (__bridge NSView *)reinterpret_cast<void *>(wid);
    nsView.layer = metalLayer;
    nsView.wantsLayer = YES;

    return (__bridge void*)metalLayer;
}
