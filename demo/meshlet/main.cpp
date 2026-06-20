#include <iostream>
#include "MeshletFrameWork.h"

int main()
{
    GNXEngine::WindowProps props("GNXEngine_Meshlet", 1280U, 720U);
    MeshletFrameWork app(props);
    app.RunLoop();
    return 0;
}
