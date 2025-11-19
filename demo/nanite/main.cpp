#include "NaniteFrameWork.h"

int main(int argc, char* argv[])
{
    GNXEngine::WindowProps props("GNXEngine_Nanite", 1400U, 480U);
    NaniteFrameWork app(props);
    app.RunLoop();
	return 0;
}
