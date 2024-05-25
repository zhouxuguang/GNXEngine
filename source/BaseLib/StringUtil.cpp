#include "StringUtil.h"

NS_BASELIB_BEGIN

void StringUtil::Split(const std::string& strSrc,const std::string& strDemlit,std::vector<std::string>& vecTokens)
{
	vecTokens.clear();
	char* pszResult = NULL;
	const char* pszDelim  = strDemlit.c_str();
	char* pszSrc = (char*)strSrc.c_str();
	pszResult = strtok(pszSrc,pszDelim);
	while (pszResult != NULL)
	{
		vecTokens.push_back(pszResult);
		pszResult = strtok(NULL,pszDelim);
	}
}

void StringUtil::Trim(std::string& strSrc)
{
	int size = (int)strSrc.size();
	int start = 0;
	int end  = size - 1;
	for( ; start < size ; ++start)
	{
		if(strSrc[start] > ' ')
		{
			break;
		}
	}
	for( ; end >= 0 ; --end)
	{
		if(strSrc[end] > ' ')
		{
			break;
		}
	}
	strSrc.erase(end+1,size-end);
	strSrc.erase(0,start);
}

/*
 * @ch is the unicode character whitespace?
 *
 * Reference: http://en.wikipedia.org/wiki/Whitespace_character#Unicode
 *
 * Return value: weather the character is a whitespace character.
 * */
bool StringUtil::IsUnicodeSpace(wchar_t ch)
{
    return  (ch >= 0x0009 && ch <= 0x000D) || ch == 0x0020 || ch == 0x0085 || ch == 0x00A0 || ch == 0x1680
    || (ch >= 0x2000 && ch <= 0x200A) || ch == 0x2028 || ch == 0x2029 || ch == 0x202F
    ||  ch == 0x205F || ch == 0x3000;
}

bool StringUtil::IsCJKUnicode(wchar_t ch)
{
    return (ch >= 0x4E00 && ch <= 0x9FBF)   // CJK Unified Ideographs
    || (ch >= 0x2E80 && ch <= 0x2FDF)   // CJK Radicals Supplement & Kangxi Radicals
    || (ch >= 0x2FF0 && ch <= 0x30FF)   // Ideographic Description Characters, CJK Symbols and Punctuation & Japanese
    || (ch >= 0x3100 && ch <= 0x31BF)   // Korean
    || (ch >= 0xAC00 && ch <= 0xD7AF)   // Hangul Syllables
    || (ch >= 0xF900 && ch <= 0xFAFF)   // CJK Compatibility Ideographs
    || (ch >= 0xFE30 && ch <= 0xFE4F)   // CJK Compatibility Forms
    || (ch >= 0x31C0 && ch <= 0x4DFF);  // Other extensions
}

NS_BASELIB_END
