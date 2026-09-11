#include "SSRFrameWork.h"

int main()
{
    GNXEngine::WindowProps props("GNXEngine_SSR", 1280U, 720U);
    SSRFrameWork app(props);
    app.RunLoop();
    return 0;
}
