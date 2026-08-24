set(SDL_VIDEO ON)
set(SDL_EVENTS ON)
set(SDL_THREADS ON)
set(SDL_LOADSO ON)

# Android: SDL_android.c unconditionally calls joystick/haptic functions,
# so these subsystems must be enabled on Android.
if(ANDROID)
    set(SDL_JOYSTICK ON)
    set(SDL_HAPTIC ON)
else()
    set(SDL_JOYSTICK OFF)
    set(SDL_HAPTIC OFF)
endif()

# 禁用所有其他功能
set(SDL2_DISABLE_INSTALL OFF)
set(SDL_AUDIO OFF)
set(SDL_SENSOR OFF)
set(SDL_POWER OFF)
# SDL_FILE：包内文件读取（SDL_RWFromFile）必需。
#   - iOS：控制 src/file/cocoa/*.m（SDL_rwopsbundlesupport.m）编译，关闭则
#     SDL_RWFromFile 的 bundle 分支（SDL_rwops.c #if !defined(SDL_FILE_DISABLED)）
#     被排除 → iOS 包内资源读取失效。必须 ON。
set(SDL_FILE ON)
set(SDL_TIMERS OFF)
set(SDL_CPUINFO OFF)
# SDL_FILESYSTEM：Android SDL_RWFromFile 内部存储回退（SDL_rwops.c 调
#   SDL_AndroidGetInternalStoragePath，实现在 src/filesystem/android）必需。关闭则
#   该符号未定义 → Android 链接风险。必须 ON。
set(SDL_FILESYSTEM ON)
set(SDL_DLOPEN OFF)
set(SDL_LOCALE OFF)
set(SDL_MISC OFF)
set(SDL_RENDER OFF)
set(SDL_OPENGL OFF)
set(SDL_OPENGLES OFF)
set(SDL_VULKAN OFF)
# iOS 需要 Metal 支持（SDL_Metal_CreateView / SDL_Metal_GetLayer 获取 CAMetalLayer）
if(IOS)
    set(SDL_METAL ON)
else()
    set(SDL_METAL OFF)
endif()
set(SDL_RENDER_D3D OFF)
set(SDL_KMSDRM OFF)
set(SDL_OFFSCREEN OFF)
set(SDL_RPI OFF)
set(SDL_COCOA OFF)  # 我们将根据平台单独启用一个后端
set(SDL_DIRECTX OFF)
set(SDL_WAYLAND OFF)
set(SDL_X11 OFF)

# 进一步禁用内部功能
set(SDL_SSE OFF)
set(SDL_SSE2 OFF)
set(SDL_AVX OFF)
set(SDL_NEON OFF)
set(SDL_ALTIVEC OFF)
set(SDL_ASSEMBLY OFF)  # 禁用汇编优化
set(SDL_OSS OFF)
set(SDL_ALSA OFF)
set(SDL_SNDIO OFF)
set(SDL_DISKAUDIO OFF)
set(SDL_DUMMYAUDIO OFF)
set(SDL_DUMMYVIDEO OFF)

set(SDL_TEST OFF)
set(SDL_SHARED OFF)

add_subdirectory(SDL)

set_property(TARGET sdl_headers_copy PROPERTY FOLDER "ThirdParty/SDL2")
set_property(TARGET SDL2main PROPERTY FOLDER "ThirdParty/SDL2")
set_property(TARGET SDL2-static PROPERTY FOLDER "ThirdParty/SDL2")
