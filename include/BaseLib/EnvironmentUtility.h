/**
* @file              EnvironmentUtility.h
* @brief             环境基本操作实用类
* @details           
* @author            周旭光 (zhouxuguang236@126.com)
* @date              2015年12月4日
* @version           1.0.0.1
* @par               Copyright (c): 2015
* @par               History:
*/

#ifndef BASELIB_ENVIRONMENTUTILITY_INCLUDE_H
#define BASELIB_ENVIRONMENTUTILITY_INCLUDE_H

#include "Singleton.h"

NS_BASELIB_BEGIN


class BASELIB_API EnvironmentUtility : public Singleton<EnvironmentUtility>
{
public:
	//获得环境变量
	std::string GetEnvironmentVariable(const std::string& strVariable) const;

	//获得当前用户所在目录
	std::string GetUserDir() const;

	//获得当前用户名
	std::string GetUserName() const;

	/**
    * @return The current working dir which is $(PWD) for unix $(CD) for
    * windows.
    */
    std::string GetCurrentWorkingDir() const;

	//获得处理器个数
	int GetProcessorCount() const;

	//获得当前用户名
	std::string GetSystemName() const;

	//获得系统的显示名称，如  windows 7  ios10
	std::string GetDisplayName() const;

	//获得当前设备的计算机名称
	std::string GetNodeName() const;

	//获得CPU架构类型名称
	std::string GetArchitecture() const;

	//获得系统版本号
	std::string GetSystemVersion() const;
};

NS_BASELIB_END

#endif
