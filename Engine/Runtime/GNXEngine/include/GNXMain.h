//
//  GNXMain.h
//  GNXEngine
//
//  统一的程序入口头文件。
//
//  用途：封装平台差异，让 demo/应用 无需手动处理入口差异，并且
//  对外隐藏 SDL2 头文件依赖：
//    - iOS/Android (GNX_WINDOW_SDL==1)：demo 仍写 int main(...)，
//      本头文件自动将其重定向为 C 链接的 SDL_main，
//      由 SDL 的 iOS/Android 后端 (SDL_UIKitRunApp / dlsym) 调用。
//      全程不 include 任何 SDL 头文件，避免把 SDL 头文件暴露给 SDK 使用者。
//    - 桌面端 (GNX_WINDOW_SDL==0)：普通 int main()，无任何影响。
//
//  用法（所有平台完全一致，demo 入口函数不用改）：
//     #include "GNXMain.h"
//
//     int main(int argc, char* argv[])
//     {
//         ... 应用逻辑 ...
//     }
//

#ifndef GNX_ENGINE_MAIN_INCLUDE_H
#define GNX_ENGINE_MAIN_INCLUDE_H

// GNX_WINDOW_SDL 由 CMake 定义（GNXEngine 通过 PUBLIC 宏传递给依赖方）
// iOS / Android → 1（使用 SDL2 窗口 + 输入）
// Windows / macOS / Linux → 0（使用 GLFW）
//
// 原理说明：
//   移动端 SDL 需要应用提供一个 C 链接的 SDL_main 符号：
//     - iOS:  SDL2main 的 int main() 里调用 SDL_UIKitRunApp(argc, argv, SDL_main)
//     - Android: SDL 的 JNI nativeRunMain 用 dlsym(lib, "SDL_main") 动态查找
//   这里不 include <SDL_main.h>，而是：
//     1. 自行声明 SDL_main 为 C 链接（等价于 SDL_main.h 里的 extern "C" 声明）
//     2. 用 #define main SDL_main 让使用者写的 int main() 预处理后变成 int SDL_main()
//        （C++ 中先有 extern "C" 声明、后有定义，定义会继承 C 链接）
//   这样 demo 代码完全不用改，且无需暴露 SDL 头文件。
#if GNX_WINDOW_SDL

// 1) 声明 SDL_main 为 C 链接（等价于 SDL_main.h 的 extern "C" 声明，无需引入 SDL 头文件）
#ifdef __cplusplus
extern "C" int SDL_main(int argc, char *argv[]);
#endif

// 2) 把使用者写的 main() 重定向为 SDL_main
//    注意：这会作用于所有包含本头文件的翻译单元中出现的 main 标识符。
//    SDL 官方 SDL_main.h 也是同样做法，风险可控。
#define main SDL_main

// 兼容：允许使用者在个别文件里显式取消重定向（如想要自己的 WinMain）
// #define SDL_MAIN_HANDLED 可放在 include 本头文件之前

#else // GNX_WINDOW_SDL == 0 (桌面端)

// 桌面端：不做任何改动，int main() 就是系统入口

#endif // GNX_WINDOW_SDL

#endif // GNX_ENGINE_MAIN_INCLUDE_H
