#include <iostream>
#include "SSAOFrameWork.h"

int main() 
{
    GNXEngine::WindowProps props("GNXEngine_SSAO", 1280U, 720U);
    SSAOFrameWork app(props);
    app.RunLoop();
    return 0;
}
