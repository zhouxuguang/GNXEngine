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

#if _WIN32
	#ifdef RENDERSYSTEM_EXPORTS
		#ifdef __GNUC__
			#define RENDERSYSTEM_API __attribute__((dllexport))
		#else
			#define RENDERSYSTEM_API __declspec(dllexport)
		#endif
	#else
		#ifdef __GNUC__
			#define RENDERSYSTEM_API __attribute__((dllimport))
		#else
			#define RENDERSYSTEM_API __declspec(dllimport)
		#endif
	#endif
	#define RENDERSYSTEM_API_HIDE
#else
	#if __GNUC__>=4
		#define RENDERSYSTEM_API __attribute__((visibility("default")))
		#define RENDERSYSTEM_API_HIDE __attribute__ ((visibility("hidden")))
	#else
		#define RENDERSYSTEM_API_HIDE
		#define RENDERSYSTEM_API
	#endif
#endif

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
