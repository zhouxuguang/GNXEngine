#include <iostream>
#include "PBRFrameWork.h"

int main()
{
    GNXEngine::WindowProps props("GNXEngine_PBR", 1280U, 720U);
    PBRFrameWork app(props);
    app.RunLoop();
    return 0;
}
