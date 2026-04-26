#include <iostream>
#include "MeshShaderFrameWork.h"

int main()
{
    GNXEngine::WindowProps props("GNXEngine_MeshShader", 1280U, 720U);
    MeshShaderFrameWork app(props);
    app.RunLoop();
    return 0;
}
