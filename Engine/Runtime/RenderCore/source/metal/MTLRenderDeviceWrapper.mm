//
//  MTLRenderDeviceWrapper.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/28.
//

#import "MTLRenderDeviceWrapper.h"
#include "MTLRenderDevice.h"

NAMESPACE_RENDERCORE_BEGIN

RenderDevicePtr createMetalRenderDevice(const NativeWindow& nativeWindow)
{
    return std::make_shared<MTLRenderDevice>((__bridge CAMetalLayer*)nativeWindow.viewHandle);
}

NAMESPACE_RENDERCORE_END
