//
//  MTLDeviceFamily.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/9/18.
//

#include "MTLDeviceFamily.h"

NAMESPACE_RENDERCORE_BEGIN

MTLDeviceFamily::MTLDeviceFamily(id<MTLDevice> device)
{
    mDevice = device;
}

MTLDeviceFamily::~MTLDeviceFamily()
{
    //
}

NAMESPACE_RENDERCORE_END
