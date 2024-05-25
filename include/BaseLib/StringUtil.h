#ifndef BASELIB_STRING_UTILITY_INCLUDE_H_HJ458KJH
#define BASELIB_STRING_UTILITY_INCLUDE_H_HJ458KJH

#include "PreCompile.h"

NS_BASELIB_BEGIN

class BASELIB_API StringUtil
{
public:
    /**
     *  字符串分割
     *  @param strSrc    原始字符串
     *  @param strDemlit 分隔符
     *  @param vecTokens 分割后的字符串数组
     */
	static void Split(const std::string& strSrc,const std::string& strDemlit,std::vector<std::string>& vecTokens);

    /**
     *  字符串去除空白
     *  @param strSrc 字符串
     */
	static void Trim(std::string& strSrc);
    
    /**
     *  判断字符是否是Unicode的空白字符
     *  @param ch 字符
     *  @return 是返回true
     */
    static bool IsUnicodeSpace(wchar_t ch);
    
    /**
     *  判断字符是否是中文、日文以及韩文
     *  @param ch 字符
     *  @return 是返回true
     */
    static bool IsCJKUnicode(wchar_t ch);

private:
	StringUtil(void);
    
    StringUtil(const StringUtil&);
    
    StringUtil& operator = (const StringUtil&);

	~StringUtil(void);
};

NS_BASELIB_END

#endif
