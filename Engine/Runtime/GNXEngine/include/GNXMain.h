//
//  GNXMain.h
//  GNXEngine
//
//  统一的程序入口头文件。
//
//  用途：封装平台差异，让 demo/应用 无需手动处理入口差异：
//    - iOS/Android (GNX_WINDOW_SDL==1)：自动引入 <SDL_main.h>，
//      并将入口函数命名为 SDL_main（由 SDL 的 iOS/Android 后端启动生命周期）
//    - 桌面端 (GNX_WINDOW_SDL==0)：普通 int main()
//
//  用法：
//     #include "GNXMain.h"
//
//     #if GNX_WINDOW_SDL
//     int SDL_main(int argc, char* argv[])
//     #else
//     int main()
//     #endif
//     {
//         ... 应用逻辑 ...
//     }
//

#ifndef GNX_ENGINE_MAIN_INCLUDE_H
#define GNX_ENGINE_MAIN_INCLUDE_H

// GNX_WINDOW_SDL 由 CMake 定义（GNXEngine 通过 PUBLIC 宏传递给依赖方）
// iOS / Android → 1（使用 SDL2 窗口 + 输入）
// Windows / macOS / Linux → 0（使用 GLFW）
#if GNX_WINDOW_SDL
#include <SDL_main.h>
#endif

#endif // GNX_ENGINE_MAIN_INCLUDE_H
