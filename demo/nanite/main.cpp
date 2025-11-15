#include "NaniteFrameWork.h"

int main(int argc, char* argv[])
{
    GNXEngine::WindowProps props;
    NaniteFrameWork app(props);
    app.RunLoop();
	return 0;
}
