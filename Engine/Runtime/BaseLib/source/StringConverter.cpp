//
//  StringConverter.cpp
//  BaseLib
//
//  Created by zhouxuguang on 16/6/18.
//  Copyright © 2016年 zhouxuguang. All rights reserved.
//

#include "StringConverter.h"

NS_BASELIB_BEGIN

bool StringConverter::NarrowToWide(const std::string& narrow, std::wstring& outWide)
{
	fs::path path = narrow;

	outWide = path.wstring();
	return true;
}

bool StringConverter::WideToNarrow(const std::wstring& wide, std::string& outNarrow)
{
	fs::path path = wide;

	outNarrow = path.string();
	return true;
}

bool StringConverter::NarrowToUTF16(const std::string& utf8, utf16String& outUtf16)
{
	fs::path path = utf8;

	outUtf16 = path.u16string();
	return true;
}

bool StringConverter::NarrowToUTF32(const std::string& utf8, utf32String& outUtf32)
{
	fs::path path = utf8;

	outUtf32 = path.u32string();
	return true;
}

bool StringConverter::UTF16ToNarrow(const utf16String& utf16, std::string& outUtf8)
{
	fs::path path = utf16;

	outUtf8 = path.string();
	return true;
}

bool StringConverter::UTF16ToUTF32(const utf16String& utf16, utf32String& outUtf32)
{
	fs::path path = utf16;

	outUtf32 = path.u32string();
	return true;
}

bool StringConverter::UTF32ToNarrow(const utf32String& utf32, std::string& outUtf8)
{
	fs::path path = utf32;

	outUtf8 = path.string();
	return true;
}

bool StringConverter::UTF32ToUTF16(const utf32String& utf32, utf16String& outUtf16)
{
	fs::path path = utf32;

	outUtf16 = path.u16string();
	return true;
}

NS_BASELIB_END
