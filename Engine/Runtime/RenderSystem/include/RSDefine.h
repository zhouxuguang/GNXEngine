//
//  RSDefine.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/29.
//

#ifndef RENDER_SYSTEM_DEFINE_INCLUDE_JFJ
#define RENDER_SYSTEM_DEFINE_INCLUDE_JFJ

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "Runtime/RenderCore/include/RenderDefine.h"
#include "Runtime/MathUtil/include/Math3DCommon.h"
#include "Runtime/BaseLib/include/BaseLib.h"

#ifdef min
    #undef min
#endif

#ifdef max
    #undef max
#endif

#ifdef __cplusplus
    #define NS_RENDERSYSTEM_BEGIN                     namespace RenderSystem {
    #define NS_RENDERSYSTEM_END                       }
    #define USING_NS_RENDERSYSTEM                     using namespace RenderSystem;
#else
    #define NS_RENDERSYSTEM_BEGIN
    #define NS_RENDERSYSTEM_END
    #define USING_NS_RENDERSYSTEM
#endif

//USING_NS_MATHUTIL
USING_NS_RENDERCORE

#endif /* RENDER_SYSTEM_DEFINE_INCLUDE_JFJ */
