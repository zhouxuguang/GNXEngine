#include "RegexExpression.h"

#ifdef WIN32
	#include "regex/regex.h"
#else
	#include <regex.h>
#endif

#include <regex>
#include <iostream>

NS_BASELIB_BEGIN

RegexExpression::RegexExpression(void)
{
}


RegexExpression::~RegexExpression(void)
{
}

bool RegexExpression::RegexParser(const std::string& srcStr, const std::string& pattern, std::vector<std::string>& vecMatchPos)
{
    vecMatchPos.clear();
#ifndef WIN32
	regex_t reg;
	if (regcomp(&reg,pattern.c_str(),REG_EXTENDED|REG_NEWLINE) != 0)		//编译失败
	{
		regfree(&reg);
		return false;
	}
    
    regmatch_t matchs[256];
    
    int nRet = regexec(&reg,srcStr.c_str(),256,matchs,REG_NOTBOL);
    if (nRet == REG_NOMATCH)
    {
        regfree(&reg);
        return false;
    }
    
    for (int i = 0; i < 256 && matchs[i].rm_so != -1; i ++)
    {
        std::string strMatch = srcStr.substr(matchs[i].rm_so, matchs[i].rm_eo - matchs[i].rm_so + 1);
        vecMatchPos.push_back(strMatch);
    }
    regfree(&reg);
    
    return true;
    
#else
	std::tr1::regex re(pattern);
	std::tr1::smatch sm;   // 存放string结果的容器
	std::tr1::regex_match(srcStr, sm, re);
	if (sm.empty())
	{
		return false;
	}

	for(int i = 0; i < sm.size(); ++i)
	{
		vecMatchPos.push_back(sm[i]);
	}
	return true;

	// regex多次
	/*str = "!!!123!!!12333!!!890!!!";
	re = std::tr1::regex("[0-9]+");
	while(std::tr1::regex_search(str, sm, re)) {
	for(int i = 0; i < sm.size(); ++i)
	std::cout<<sm[i] << " ";
	std::cout<< std::endl;
	str = sm.suffix().str();
	}*/
    
#endif

	return 1;
}

NS_BASELIB_END
