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

#endif