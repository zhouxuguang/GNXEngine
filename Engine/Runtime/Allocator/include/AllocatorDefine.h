#ifndef GNX_ENGINE_ALLOCATOR_DEFINE_INCLUDE_SGDS
#define GNX_ENGINE_ALLOCATOR_DEFINE_INCLUDE_SGDS

#include "Runtime/BaseLib/include/BaseLib.h"

#ifdef __cplusplus
#define NS_ALLOCATOR_BEGIN                     namespace Allocator {
#define NS_ALLOCATOR_END                       }
#define USING_NS_ALLOCATOR                     using namespace Allocator;
#else
#define NS_ALLOCATOR_BEGIN
#define NS_ALLOCATOR_END
#define USING_NS_ALLOCATOR
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
	#if defined(ALLOCATOR_EXPORTS) || defined(GNXENGINE_EXPORTS)
		#define ALLOCATOR_API __declspec(dllexport)
	#else
		#define ALLOCATOR_API __declspec(dllimport)
	#endif
#else
	#if __GNUC__ >= 4
		#define ALLOCATOR_API __attribute__((visibility("default")))
	#else
		#define ALLOCATOR_API
	#endif
#endif

#endif
