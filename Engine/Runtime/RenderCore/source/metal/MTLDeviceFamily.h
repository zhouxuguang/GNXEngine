//
//  MTLDeviceFamily.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/9/18.
//

#ifndef GNX_ENGINE_MTL_DEVICE_FAMILY_ICLUDE_DGMGK
#define GNX_ENGINE_MTL_DEVICE_FAMILY_ICLUDE_DGMGK

#include "MTLRenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLDeviceFamily
{
public:
    MTLDeviceFamily(id<MTLDevice> device);
    
    ~MTLDeviceFamily();
    //
    
private:
    id<MTLDevice> mDevice = nil;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_DEVICE_FAMILY_ICLUDE_DGMGK */
