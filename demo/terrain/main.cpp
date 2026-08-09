#include "TerrainFrameWork.h"
#include "Runtime/GNXEngine/include/GNXMain.h"

// 入口函数：所有平台统一写 int main()。
// 移动端 (GNX_WINDOW_SDL==1) 时，GNXMain.h 会自动把它重定向为
// C 链接的 SDL_main（由 SDL 的 iOS/Android 后端调用），demo 无需感知。
int main(int argc, char* argv[])
{
    GNXEngine::WindowProps props("GNXEngine_Terrain", 1280U, 720U);
    TerrainFrameWork app(props);
    app.RunLoop();
    return 0;
}
