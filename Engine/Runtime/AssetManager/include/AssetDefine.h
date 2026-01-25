//
//  AssetDefine.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/29.
//

#ifndef GNXENGINE_INCLUDE_ASSET_DEFINE_INCLS
#define GNXENGINE_INCLUDE_ASSET_DEFINE_INCLS

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "Runtime/MathUtil/include/Vector2.h"
#include "Runtime/MathUtil/include/Vector4.h"
#include "Runtime/BaseLib/include/BaseLib.h"

#if _WIN32
	#ifdef __GNUC__
		#define ASSET_MANAGER_API __attribute__((dllexport))
	#else
		#define ASSET_MANAGER_API __declspec(dllexport)
	#endif
	#define ASSET_MANAGER_API_HIDE
#else
	#if __GNUC__>=4
		#define ASSET_MANAGER_API __attribute__((visibility("default")))
		#define ASSET_MANAGER_API_HIDE __attribute__ ((visibility("hidden")))
	#else
		#define ASSET_MANAGER_API_HIDE
		#define ASSET_MANAGER_API
	#endif
#endif

#ifdef min
	#undef min
#endif

#ifdef max
	#undef max
#endif

#ifdef __cplusplus
#define NS_ASSETMANAGER_BEGIN                     namespace AssetManager {
#define NS_ASSETMANAGER_END                       }
#define USING_NS_ASSETMANAGER                     using namespace AssetManager;
#else
#define NS_ASSETMANAGER_BEGIN
#define NS_ASSETMANAGER_END
#define USING_NS_ASSETMANAGER
#endif

USING_NS_MATHUTIL


#endif /* GNXENGINE_INCLUDE_ASSET_DEFINE_INCLS */