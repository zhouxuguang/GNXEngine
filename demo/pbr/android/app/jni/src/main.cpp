#include <SDL.h>
#include "PBRFrameWork.h"

extern "C" int SDL_main(int argc, char* argv[])
{
    GNXEngine::WindowProps props("GNXEngine_PBR", 1280U, 720U);
    PBRFrameWork app(props);
    app.RunLoop();
    return 0;
}
