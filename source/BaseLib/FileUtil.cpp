//
//  FileUtil.cpp
//  BaseLib
//
//  Created by Zhou,Xuguang on 17/5/12.
//  Copyright © 2017年 Zhou,Xuguang. All rights reserved.
//

#include "FileUtil.h"

#if defined(_WIN32)
#  include <io.h>
#  include <direct.h>
#  include <sys/utime.h>
#  include <sys/stat.h>
#else
#  include <sys/types.h>
#  include <utime.h>
#  include <sys/stat.h>
#  include <unistd.h>
#  include <dirent.h>
#  include <fcntl.h>
#  include <string.h>
#endif
#include <fstream>

#ifndef WIN32

static void _split_whole_name(const char *whole_name, char *fname, char *ext);

static void _splitpath(const char *path, char *drive, char *dir, char *fname, char *ext)
{
    if (strcmp(path,"") == 1)
    {
        return;
    }
    
    char *p_whole_name;
    
    drive[0] = '\0';
    if (NULL == path)
    {
        dir[0] = '\0';
        fname[0] = '\0';
        ext[0] = '\0';
        return;
    }
    
    if ('/' == path[strlen(path)])
    {
        strcpy(dir, path);
        fname[0] = '\0';
        ext[0] = '\0';
        return;
    }
    
    p_whole_name = strrchr((char*)path, '/');
    if (NULL != p_whole_name)
    {
        p_whole_name++;
        _split_whole_name(p_whole_name, fname, ext);
        
        snprintf(dir, p_whole_name - path, "%s", path);
    }
    else
    {
        _split_whole_name(path, fname, ext);
        dir[0] = '\0';
    }
}

static void _split_whole_name(const char *whole_name, char *fname, char *ext)
{
    char *p_ext;
    
    p_ext = strrchr((char*)whole_name, '.');
    if (NULL != p_ext)
    {
        strcpy(ext, p_ext);
        snprintf(fname, p_ext - whole_name + 1, "%s", whole_name);
    }
    else
    {
        ext[0] = '\0';
        strcpy(fname, whole_name);
    }
}

#endif


NS_BASELIB_BEGIN

bool FileUtil::IsExist(const std::string& strFileName)
{
    bool result = false;
#if defined(_WIN32)
    result = (_access(strFileName.c_str(), FileUtil::BL_EXIST) == 0);
#else
    result = ((access(strFileName.c_str(), FileUtil::BL_EXIST)) == 0);
#endif
    return result;
}

bool FileUtil::IsFile(const std::string &strFileName)
{
#if defined(_WIN32)
    
    struct _stat sbuf;
    if ( _stat(strFileName.c_str(), &sbuf ) == -1)
        return false;
    return (_S_IFMT & sbuf.st_mode ? true : false);
#else
    struct stat sbuf;
    
    stat(strFileName.c_str(), &sbuf);
    return ((sbuf.st_mode & S_IFMT) == S_IFREG);
#endif
}

bool FileUtil::IsDir(const std::string &strFileName)
{
    if ( strFileName.empty() )
    {
        return false;
    }
    
    std::string temp = strFileName;
    
    std::string::size_type nLastPos = temp.size()-1;
    const char& lastChar = temp[nLastPos];
    if ( lastChar == '/' || lastChar == '\\' )
    {
        if(nLastPos < temp.length())
        {
            temp.erase(nLastPos, std::string::npos);
        }
    }
    
#if defined(_WIN32)
    
    struct _stat sbuf;
    if ( _stat(temp.c_str(), &sbuf ) == -1)
        return false;
    return (_S_IFDIR & sbuf.st_mode ? true : false);
#else
    struct stat sbuf;
    if (stat(temp.c_str(), &sbuf) == -1)
        return false;
    return (S_ISDIR(sbuf.st_mode));
#endif
}

bool FileUtil::IsReadable(const std::string &strFileName)
{
#if defined(_WIN32)
    
    struct _stat sbuf;
    if ( _stat(strFileName.c_str(), &sbuf ) == -1)
        return false;
    return (_S_IREAD & sbuf.st_mode ? true : false);
#else
    return (access(strFileName.c_str(), FileUtil::BL_READ) == 0);
#endif
}

bool FileUtil::IsWriteable(const std::string &strFileName)
{
#if defined(_WIN32)
    
    struct _stat sbuf;
    if ( _stat(strFileName.c_str(), &sbuf ) == -1)
        return false;
    return (_S_IWRITE & sbuf.st_mode ? true : false);
#else
    return (access(strFileName.c_str(), FileUtil::BL_WRITE) == 0);
#endif
}

bool FileUtil::IsExecutable(const std::string &strFileName)
{
#if defined(_WIN32)
    
    struct _stat sbuf;
    if ( _stat(strFileName.c_str(), &sbuf ) == -1)
        return false;
    return (_S_IEXEC & sbuf.st_mode ? true : false);
#else
    return (access(strFileName.c_str(), FileUtil::BL_EXE) == 0);
#endif
}

int64_t FileUtil::GetFileSize(const std::string &strFileName)
{
    return fs::file_size(strFileName);
}

bool FileUtil::IsRelative(const std::string &strFileName)
{
    bool result = true;
    if (!strFileName.empty())
    {
        //---
        // Look for unix "/"...
        // ESH: Look for Windows "\" (with prepending escape character \)
        //---
        if ( (*(strFileName.begin()) == '/') || (*(strFileName.begin()) == '\\') )
        {
            result = false;
        }
        else
        {
            // Look for windows drive
            if (strFileName.size() < 2)
            {
                return true;
            }
            
            else
            {
                char cBegin = strFileName[0];
                char cSecond = strFileName[1];
                if ( ((cBegin >= 'a' && cBegin <= 'z') || (cBegin >= 'A' && cBegin <= 'Z')) && cSecond == ':')
                {
                    return false;
                }
            }
        }
    }
    return result;
}

bool FileUtil::Remove(const std::string& strPathName)
{
    bool result = true;
    
#if defined(__VISUALC__)  || defined(__BORLANDC__) || defined(__WATCOMC__) || \
defined(__GNUWIN32__) || defined(_MSC_VER)
    
    if(FileUtil::IsDir(strPathName))
    {
        // Note this only removes empty directories.
        result = ( RemoveDirectory( strPathName.c_str() ) != 0 );
    }
    else
    {
        result = ( DeleteFile( strPathName.c_str() ) != 0 );
    }
#else /* Unix flavor from unistd.h. */
    if(FileUtil::IsDir(strPathName))
    {
        result = ( rmdir( strPathName.c_str() ) == 0 );
    }
    else
    {
        result = ( unlink( strPathName.c_str() ) == 0 );
    }
#endif
    
    return result;
}

bool FileUtil::MakeDirectory(const std::string &directoryPath)
{
    if (directoryPath.empty())
    {
        return false;
    }

    std::error_code ec;
    fs::create_directories(directoryPath, ec);
    return !ec;
}

std::vector<uint8_t> FileUtil::ReadBinaryFile(const std::string& path)
{
	if (!IsFile(path)) return {};

	// 打开文件并定位到末尾
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open()) return {};

	// 获取文件大小
	size_t size = static_cast<size_t>(file.tellg());
	if (size == 0) return {};

	// 创建足够容纳文件的向量
	std::vector<uint8_t> buffer(size);

	// 回到文件开头并读取内容
	file.seekg(0);
	if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
	{
		return {}; // 读取失败
	}

	return buffer;
}

bool FileUtil::WriteBinaryFile(const std::string& path, const void* data, size_t size) 
{
	if (size == 0 || nullptr == data)
	{
		return false; // 避免创建0字节文件
	}

	// 确保目录存在
	auto dir = fs::path(path).parent_path();
	if (!dir.empty() && !fs::exists(dir))
	{
		if (!MakeDirectory(dir.string()))
		{
			return false;
		}
	}

	// 以二进制模式写入
	std::ofstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		return false;
	}

	if (!file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size)))
	{
		return false;
	}

	return file.good();
}

bool FileUtil::WriteBinaryFile(const std::string& path, const std::vector<uint8_t>& data)
{
    return WriteBinaryFile(path, data.data(), data.size());
}

NS_BASELIB_END
