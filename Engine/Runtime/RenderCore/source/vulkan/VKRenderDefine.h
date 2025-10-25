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
#elif __ANDROID__
    #define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__) || defined(__unix__)
    #define VK_USE_PLATFORM_XLIB_KHR
#elif defined(__APPLE__)
    #define VK_USE_PLATFORM_METAL_EXT
#else
#endif

#define VK_ENABLE_BETA_EXTENSIONS
#define VK_NO_PROTOTYPES
#include "volk.h"
#include "vulkan/vulkan_beta.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include "vk_mem_alloc.h"
#include "RenderDefine.h"
#include "BaseLib/DebugBreaker.h"
#include "BaseLib/BaseLib.h"

#ifdef VK_USE_PLATFORM_WIN32_KHR
#ifdef min
    #undef min
#endif
#ifdef max
    #undef max
#endif
#endif

#include <assert.h>
#include <iostream>

NAMESPACE_RENDERCORE_BEGIN

//#define ENABLE_VULKAN_MSAA  //开启msaa
#define ENABLE_VULKAN_DEBUG   //开启DEBUG

#define ENABLE_VK_RESULT_CHECK  //开启返回值检查

/**
 * @brief Helper function to convert a VkResult enum to a string
 * @param result Vulkan result to convert.
 * @return The string to return.
 */
const std::string toString(VkResult result);

#ifdef ENABLE_VK_RESULT_CHECK
    #define VK_CHECK(x)                                                                           \
        do                                                                                        \
        {                                                                                         \
            VkResult err = x;                                                                     \
            if (err)                                                                              \
            {                                                                                     \
                std::cerr << "Detected Vulkan error: " + toString(err);                           \
                baselib::DebugBreak();                                                            \
            }                                                                                     \
        } while (0)
#else
    #define VK_CHECK(x) x
#endif

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_RENDER_DEFINE_INCLUDE_GKDFGJ */
