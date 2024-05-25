/**
* @file              FileName.h
* @brief             文件路径基本操作的类
* @details           
* @author            周旭光 (zhouxuguang236@126.com)
* @date              2015年12月4日
* @version           1.0.0.1
* @par               Copyright (c): 2015
* @par               History:
*/

#ifndef BASELIB_FILENAME_INCLUDE_H
#define BASELIB_FILENAME_INCLUDE_H

#include "PreCompile.h"

NS_BASELIB_BEGIN

class BASELIB_API FileName
{
public:

	enum AccessModes
	{
		BL_READ_WRITE = 6,
		BL_READ       = 4,
		BL_WRITE      = 2,
		BL_EXE	      = 1, 
		BL_EXIST      = 0
	};

	FileName(void);

	FileName(const FileName& src);

	FileName(const std::string& src);

	FileName(const char* src);

	~FileName(void);

	bool operator == (const FileName& rhs) const;

	//bool operator == (const ossimString& rhs)const;
	//bool operator == (const char* rhs)const;

	std::string GetFileName() const;

	void SetFileName(const std::string& strFileName);

	/**
    * 获得文件路径扩展之后的路径
    *
    * Like: ~/docs will be expanded to /home/user/docs
    *
    * @return The expanded file name.
    *
    * @note METHOD IS NOT COMPLETE YET.
    */
   FileName Expand() const;


	//检测文件的属性

	//文件是否存在
	bool Exists() const;

	bool IsFile() const;

	bool IsDir() const;

	bool IsReadable()  const;

	bool IsWriteable()  const;

	bool IsExecutable() const;

	int64_t GetFileSize() const;

	/**
    * @brief Checks whether file name is relative or absolute.
    * 
    * @return true if file name has a relative path, false if absolute.
    *
    * @note This will return true if file name is empty.
    */
   bool IsRelative() const;

   /**
    * @brief Method to check if expansion is needed.
    *
    * This checks if file IsRelative() first if isRelative is false (has
    * absolute path) it then walks the file and looks for '$'.
    * 
    * @return true if file name is relative or has environment vars.
    *
    * @note This will return false if file name is empty.
    */
   bool IsNeedExpansion() const;


	// 访问文件字符串的一部分

	/**
	* @return "tif" if file is "/data/images/t1.tif".
	*
	* @note The '.' (dot) is not returned.
	*/
	std::string  GetExtention() const;

	/**
	* @return @return "/data/images" if file is "/data/images/t1.tif".
	*/
	std::string GetPath() const;

	/**
	* @return @return "c:" if file is "c:\data\images\t1.tif".
	*/
	std::string GetDrive() const;

	/**
	* @return @return "t1.tif" if file is "/data/images/t1.tif".
	*/
	std::string GetFile() const;

	/**
	* @return @return "t1" if file is "/data/images/t1.tif".
	*/
	std::string GetFileNoExtension() const;

	/**
	* @return @return "/data/images/t1" if file is "/data/images/t1.tif".
	*
	* @note  The '.' (dot) is not returned.
	*/
	std::string GetNoExtension() const;

	//设置文件后缀
	std::string SetExtension(const std::string& e);

	//移除这个路径的内容
	bool Remove() const;

	//创建文件夹
	bool CreateDirectory(bool bRecurseFlag=true,int nPerm=0775) const;

	//分裂和合并方法
	void Split(std::string& drivePart,
		std::string& pathPart,
		std::string& filePart,
		std::string& extPart) const;

	void Merge(const std::string& drivePart,
		const std::string& pathPart,
		const std::string& filePart,
		const std::string& extPart);

	/*!
	* @brief Removes pathname from filesystem if supported by platform.
	*
	* If pathname is a directory it will only be removed if empty. If supported
	* will set errno and output equivalent string if one occurs.
	*
	* @return true on success, false on error.
	*/
	static bool Remove(const std::string& strPathName);

	//重命名
	bool Rename(const std::string& strDestFile, bool overwriteDestinationFlag=true) const;

private:
	std::string m_strFileName;//文件名

};

NS_BASELIB_END

#endif
