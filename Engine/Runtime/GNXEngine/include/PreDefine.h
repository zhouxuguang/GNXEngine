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

#endif
