#include <iostream>
#include "LumenFrameWork.h"

int main() 
{
    GNXEngine::WindowProps props("GNXEngine_Lumen", 1280U, 720U);
    LumenFrameWork app(props);
    app.RunLoop();
	return 0;
}