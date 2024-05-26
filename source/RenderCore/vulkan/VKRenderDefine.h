//
//  VKRenderDefine.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/23.
//

#ifndef GNX_ENGINE_VK_RENDER_DEFINE_INCLUDE_GKDFGJ
#define GNX_ENGINE_VK_RENDER_DEFINE_INCLUDE_GKDFGJ

#if defined(_WIN32)
    #define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__) || defined(__unix__)
    #define VK_USE_PLATFORM_XLIB_KHR
#elif defined(__APPLE__)
    #define VK_USE_PLATFORM_MACOS_MVK
#else
#endif
#define VK_NO_PROTOTYPES
#include "volk.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include "vk_mem_alloc.h"
#include "RenderDefine.h"

#include <assert.h>

//#define ENABLE_VULKAN_MSAA  //开启msaa
#define ENABLE_VULKAN_DEBUG   //开启DEBUG

#endif /* GNX_ENGINE_VK_RENDER_DEFINE_INCLUDE_GKDFGJ */
