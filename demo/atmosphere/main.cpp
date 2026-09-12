#include "AtmosphereFrameWork.h"
#include "Runtime/GNXEngine/include/GNXMain.h"

int main(int argc, char* argv[])
{
    GNXEngine::WindowProps props("GNXEngine_Atmosphere", 1280U, 720U);
    AtmosphereFrameWork app(props);
    app.RunLoop();
    return 0;
}
