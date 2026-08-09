// ============================================================
// terrain Android 入口
// Android 上 SDL 要求入口为 SDL_main（由 SDLActivity → SDL 的
// native main 机制调用），然后在内部创建引擎窗口并启动主循环。
// ============================================================

#include <SDL.h>

// 引擎窗口 + 地形框架
#include "Runtime/GNXEngine/include/AppFrameWork.h"
#include "TerrainFrameWork.h"

extern "C" int SDL_main(int argc, char* argv[])
{
    // Android 上窗口大小由屏幕决定，这里只是初始值
    GNXEngine::WindowProps props("GNXEngine_Terrain", 1280U, 720U);
    TerrainFrameWork app(props);
    app.RunLoop();
    return 0;
}
