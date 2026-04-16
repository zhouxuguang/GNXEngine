#include "TerrainFrameWork.h"

int main()
{
    GNXEngine::WindowProps props("GNXEngine_Terrain", 1280U, 720U);
    TerrainFrameWork app(props);
    app.RunLoop();
}
