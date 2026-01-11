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

    std::u16string s1;
    baselib::StringConverter::NarrowToUTF16(r1, s1);

    std::string s2;
    baselib::StringConverter::UTF16ToNarrow(s1, s2);
    
    std::wstring s3;
    baselib::StringConverter::NarrowToWide(s2, s3);
    
    std::string s4;
    baselib::StringConverter::WideToNarrow(s3, s4);
    
    assert(s2 == r1);

    GNXEngine::WindowProps props("GNXEngine_Nanite", 1400U, 480U);
    NaniteFrameWork app(props);
    app.RunLoop();
	return 0;
}
