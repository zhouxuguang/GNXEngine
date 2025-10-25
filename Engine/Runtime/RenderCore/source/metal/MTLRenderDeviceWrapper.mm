//
//  MTLRenderDeviceWrapper.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/28.
//

#import "MTLRenderDeviceWrapper.h"
#include "MTLRenderDevice.h"

NAMESPACE_RENDERCORE_BEGIN

RenderDevicePtr createMetalRenderDevice(void* windowPtr)
{
    return std::make_shared<MTLRenderDevice>((__bridge CAMetalLayer*)windowPtr);
}

NAMESPACE_RENDERCORE_END
