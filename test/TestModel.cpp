
#include "Runtime/GNXEngine/include/AppFrameWork.h"

int main(int argc, char* argv[])
{
    GNXEngine::WindowProps props;
    GNXEngine::AppFrameWork app(props);
    app.RunLoop();
	return 0;
}
