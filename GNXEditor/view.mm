//
//  View.cpp
//  testQT
//
//  Created by zhouxuguang on 2022/8/6.
//

#include "View.h"
#include <MetalKit/MetalKit.h>
#include "MetalView.h"

void* test()
{
    NSRect rect;
    rect.origin.x = 0;
    rect.origin.y = 0;
    rect.size.width = 0;
    rect.size.height = 0;
    MetalView *view = [[MetalView alloc] initWithFrame: rect];
    
    return (__bridge_retained void*)view;
}
