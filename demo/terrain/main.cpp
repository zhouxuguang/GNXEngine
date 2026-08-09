#include "TerrainFrameWork.h"

int main(int argc, char* argv[])
{
    GNXEngine::WindowProps props("GNXEngine_Terrain", 1280U, 720U);
    TerrainFrameWork app(props);
    app.RunLoop();
    return 0;
}
