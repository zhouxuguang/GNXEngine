/**
 * 游戏引擎预定义头文件
 */

#ifndef GNX_ENGINE_PREDEFINE_JDGJDFJVGNDFKN_INCLUDE
#define GNX_ENGINE_PREDEFINE_JDGJDFJVGNDFKN_INCLUDE

#define NAMESPACE_GNXENGINE_BEGIN        namespace GNXEngine {
/** namespace结束宏 */
#define NAMESPACE_GNXENGINE_END            }

#define USING_NS_GNXENGINE                     using namespace GNXEngine;

#include "Runtime/BaseLib/include/BaseLib.h"

//导出宏定义
#if defined _WIN32 || defined __CYGWIN__ || _WIN64
	#ifdef GNXENGINE_EXPORTS		
		#ifdef __GNUC__
			#define GNXENGINE_API __attribute__((dllexport))
		#else
			#define GNXENGINE_API __declspec(dllexport)
		#endif
	#else
		#ifdef __GNUC__
			#define GNXENGINE_API __attribute__((dllimport))
		#else
			#define GNXENGINE_API __declspec(dllimport)
		#endif
	#endif
	#define GNXENGINE_API_HIDE
#else
	#if __GNUC__>=4
		#define GNXENGINE_API __attribute__((visibility("default")))
		#define GNXENGINE_API_HIDE __attribute__ ((visibility("hidden")))
	#else
		#define GNXENGINE_API
		#define GNXENGINE_API_HIDE
	#endif
#endif

// 注意：GNXMain.h（入口宏 #define main SDL_main）不应在这里 include，
// 因为它会在所有包含本头文件的翻译单元中重定向 main 标识符。
// 应改为由应用的 main.cpp 显式 include GNXMain.h，只在应用入口生效。

#endif
