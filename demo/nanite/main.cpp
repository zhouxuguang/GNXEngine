#include "NaniteFrameWork.h"

#include <iostream>  
#include <string>  
#include <locale>  
#include <codecvt>

#include "Runtime/BaseLib/include/StringConverter.h"

int main(int argc, char* argv[])
{
    GNXEngine::WindowProps props("GNXEngine_Nanite", 1280U, 720U);
    NaniteFrameWork app(props);
    app.RunLoop();
	return 0;
}
