#include <iostream>
#include "VTFrameWork.h"

int main()
{
    GNXEngine::WindowProps props("GNXEngine_VirtualTexture", 1280U, 720U);
    VTFrameWork app(props);
    app.RunLoop();
    return 0;
}