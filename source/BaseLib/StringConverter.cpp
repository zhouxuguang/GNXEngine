//
//  StringConverter.cpp
//  BaseLib
//
//  Created by zhouxuguang on 16/6/18.
//  Copyright © 2016年 zhouxuguang. All rights reserved.
//

#include "StringConverter.h"
#include "ConvertUTF.h"

NS_BASELIB_BEGIN


//utf8,utf16,utf32之间的转换
//Trait  待研究
template <typename T>
struct ConvertTrait {
    typedef T ArgType;
};
template <>
struct ConvertTrait<char> {
    typedef UTF8 ArgType;
};
template <>
struct ConvertTrait<uint16_t> {
    typedef UTF16 ArgType;
};
template <>
struct ConvertTrait<uint32_t> {
    typedef UTF32 ArgType;
};

//c++ 98在模板函数中不支持默认模板参数
template <typename From, typename To, typename FromTrait, typename ToTrait >
static bool utfConvert(
                const std::basic_string<From>& from, std::basic_string<To>& to,
                ConversionResult(*cvtfunc)(const FromTrait**, const FromTrait*,
                                           ToTrait**, ToTrait*,
                                           ConversionFlags)
                )
{
    assert(sizeof(From) == sizeof(FromTrait));
    assert(sizeof(To) == sizeof(ToTrait));
    
    if (from.empty())
    {
        to.clear();
        return true;
    }
    
    // See: http://unicode.org/faq/utf_bom.html#gen6
    const int most_bytes_per_character = 4;
    
    const size_t maxNumberOfChars = from.length(); // all UTFs at most one element represents one character.
    const size_t numberOfOut = maxNumberOfChars * most_bytes_per_character / sizeof(To);
    
    std::basic_string<To> working(numberOfOut, 0);

    const FromTrait* inbeg = reinterpret_cast<const FromTrait*>(&from[0]);
    const FromTrait* inend = inbeg + from.length();
    
    
    ToTrait* outbeg = reinterpret_cast<ToTrait*>(&working[0]);
    ToTrait* outend = outbeg + working.length();
    ConversionResult result = cvtfunc(&inbeg, inend, &outbeg, outend, strictConversion);
    if (result != conversionOK)
        return false;
    
    working.resize(reinterpret_cast<To*>(outbeg) - (To*)&working[0]);
    //to = std::move(working);
    to = working;
    
    return true;
};

bool StringConverter::UTF8ToUTF16(const std::string& utf8, utf16String& outUtf16)
{
    return utfConvert<char, uint16_t,UTF8,UTF16>(utf8, outUtf16, ConvertUTF8toUTF16);
}

bool StringConverter::UTF8ToUTF32(const std::string& utf8, utf32String& outUtf32)
{
    return utfConvert<char,uint32_t,UTF8,UTF32>(utf8, outUtf32, ConvertUTF8toUTF32);
}

bool StringConverter::UTF16ToUTF8(const utf16String& utf16, std::string& outUtf8)
{
    return utfConvert< uint16_t,char,UTF16,UTF8>(utf16, outUtf8, ConvertUTF16toUTF8);
}

bool StringConverter::UTF16ToUTF32(const utf16String& utf16, utf32String& outUtf32)
{
    return utfConvert< uint16_t,uint32_t,UTF16,UTF32>(utf16, outUtf32, ConvertUTF16toUTF32);
}

bool StringConverter::UTF32ToUTF8(const utf32String& utf32, std::string& outUtf8)
{
    return utfConvert<uint32_t,char,UTF32,UTF8>(utf32, outUtf8, ConvertUTF32toUTF8);
}

bool StringConverter::UTF32ToUTF16(const utf32String& utf32, utf16String& outUtf16)
{
    return utfConvert<uint32_t, uint16_t,UTF32,UTF16>(utf32, outUtf16, ConvertUTF32toUTF16);
}

NS_BASELIB_END
