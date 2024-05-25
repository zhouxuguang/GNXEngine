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
#include "RenderCore/RenderDefine.h"
#include "MathUtil/Math3DCommon.h"

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
