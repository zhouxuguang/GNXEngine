#include "NaniteFrameWork.h"

#include <iostream>  
#include <string>  
#include <locale>  
#include <codecvt>

#include "Runtime/BaseLib/include/StringConverter.h"

int main(int argc, char* argv[])
{
    //std::wstring str = ;
    fs::path path = fs::u8path("D:\\BaiduNetdiskDownload\\xx86OpenGLVulkan图形学光线追踪相关（电子版）");
    
    std::string r1 = path.string();
    std::wstring r2 = path.wstring();
    std::u16string r3 = path.u16string();
    std::u32string r4 = path.u32string();

    std::u16string s1;
    baselib::StringConverter::UTF8ToUTF16(r1, s1);

    std::string s2;
    baselib::StringConverter::UTF16ToUTF8(s1, s2);
    
    assert(s2 == r1);

    GNXEngine::WindowProps props("GNXEngine_Nanite", 1400U, 480U);
    NaniteFrameWork app(props);
    app.RunLoop();
	return 0;
}
