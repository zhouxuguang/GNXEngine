//
//  StringConverter.h 字符串转换的类
//  BaseLib
//
//  Created by zhouxuguang on 16/6/18.
//  Copyright © 2016年 zhouxuguang. All rights reserved.
//

#ifndef BASELIB_STRINGCONVERTER_INCLUDE_H
#define BASELIB_STRINGCONVERTER_INCLUDE_H

#include "PreCompile.h"

NS_BASELIB_BEGIN

class BASELIB_API StringConverter
{
public:
    
    /**
     *  narrow->wide
     *  @param narrow     narrow
     *  @param outWide  wide
     *  @return 成功返回1
     */
    static bool NarrowToWide(const std::string& narrow, std::wstring& outWide);
    
    /**
     *  wide->narrow
     *  @param wide     wide
     *  @param outNarrow  narrow
     *  @return 成功返回1
     */
    static bool WideToNarrow(const std::wstring& wide, std::string& outNarrow);
    
    /**
     *  narrow->utf16
     *  @param narrow     narrow
     *  @param outUtf16 utf16
     *  @return 成功返回1
     */
    static bool NarrowToUTF16(const std::string& narrow, utf16String& outUtf16);
    
    /**
     *  narrow->utf32
     *  @param utf8     utf8
     *  @param outUtf32 utf32
     *  @return 成功返回1
     */
    static bool NarrowToUTF32(const std::string& narrow, utf32String& outUtf32);
    
    /**
     *  utf16->narrow
     *  @param utf16   utf16
     *  @param outNarrow narrow
     *  @return 成功返回1
     */
    static bool UTF16ToNarrow(const utf16String& utf16, std::string& outNarrow);
    
    /**
     *  utf16->utf32
     *  @param utf16    utf16
     *  @param outUtf32 utf32
     *  @return 成功返回1
     */
    static bool UTF16ToUTF32(const utf16String& utf16, utf32String& outUtf32);
    
    /**
     *  utf32->utf8
     *  @param utf32   utf32
     *  @param outNarrow narrow
     *  @return 成功返回1
     */
    static bool UTF32ToNarrow(const utf32String& utf32, std::string& outNarrow);
    
    /**
     *  utf32->utf16
     *  @param utf32    utf32
     *  @param outUtf16 utf16
     *  @return 成功返回1
     */
    static bool UTF32ToUTF16(const utf32String& utf32, utf16String& outUtf16);
    
private:
    StringConverter();
    StringConverter(const StringConverter& );
    StringConverter& operator=(const StringConverter& );
    ~StringConverter();
};

NS_BASELIB_END

#endif /* BASELIB_STRINGCONVERTER_INCLUDE_H */
