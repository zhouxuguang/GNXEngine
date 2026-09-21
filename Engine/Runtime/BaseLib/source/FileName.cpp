#include "FileName.h"

#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

#if defined(_WIN32)
#  include <io.h>
#  include <direct.h>
#  include <sys/utime.h>
#  include <windows.h>
#else
#  include <sys/types.h>
#  include <utime.h>
#  include <sys/stat.h>
#  include <unistd.h>
#  include <dirent.h>
#  include <fcntl.h>
#  include <string.h>
#endif

#include <sys/stat.h>

#include "EnvironmentUtility.h"
#include "StringUtil.h"

NS_BASELIB_BEGIN

FileName::FileName(void)
{
	m_strFileName.clear();
}

FileName::FileName(const FileName& src)
{
	m_strFileName = src.m_strFileName;
}

FileName::FileName(const std::string& src)
{
	m_strFileName = src;
}

FileName::FileName(const char* src)
{
	m_strFileName = src != nullptr ? src : "";
}

FileName::~FileName(void)
{
	m_strFileName.clear();
}

bool FileName::Exists() const
{
	bool result = false;
#if defined(_WIN32)
	result = (_access(m_strFileName.c_str(), FileName::BL_EXIST) == 0);
#else
	result = ((access(m_strFileName.c_str(), FileName::BL_EXIST)) == 0);
#endif
	return result;
}

bool FileName::IsFile() const
{
#if defined(_WIN32)

	struct _stat sbuf;
	if ( _stat(m_strFileName.c_str(), &sbuf ) == -1)
		return false;
	return (sbuf.st_mode & _S_IFMT) == _S_IFREG;
#else
	struct stat sbuf;

	if (stat(m_strFileName.c_str(), &sbuf) != 0)
		return false;
	return ((sbuf.st_mode & S_IFMT) == S_IFREG);
#endif
}

bool FileName::IsDir() const
{
	if ( m_strFileName.empty() )
	{
		return false;
	}

	std::string temp = m_strFileName;

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

bool FileName::IsReadable() const
{
#if defined(_WIN32)

	struct _stat sbuf;
	if ( _stat(m_strFileName.c_str(), &sbuf ) == -1)
		return false;
	return (_S_IREAD & sbuf.st_mode ? true : false);
#else
	return (access(m_strFileName.c_str(), FileName::BL_READ) == 0);
#endif
}

bool FileName::IsWriteable() const
{
#if defined(_WIN32)

	struct _stat sbuf;
	if ( _stat(m_strFileName.c_str(), &sbuf ) == -1)
		return false;
	return (_S_IWRITE & sbuf.st_mode ? true : false);
#else
	return (access(m_strFileName.c_str(), FileName::BL_WRITE) == 0);
#endif
}

bool FileName::IsExecutable() const
{
#if defined(_WIN32)

	struct _stat sbuf;
	if ( _stat(m_strFileName.c_str(), &sbuf ) == -1)
		return false;
	return (_S_IEXEC & sbuf.st_mode ? true : false);
#else
	return (access(m_strFileName.c_str(), FileName::BL_EXE) == 0);
#endif
}

int64_t FileName::GetFileSize() const
{
	struct stat sbuf;

#ifndef __BORLANDC__
	if (stat(m_strFileName.c_str(), &sbuf) == 0)
	{
		return (int64_t)sbuf.st_size;
	}
	else
	{
		ifstream in(m_strFileName.c_str());
		if(in)
		{
			in.seekg(SEEK_END);
			return (int64_t)in.tellg();
		}
	}
#else
	ifstream in(m_strFileName.c_str());
	if(in)
	{
		in.seekg(SEEK_END);
		return (int64_t)in.tellg();
	}
#endif
	return 0;
}

bool FileName::IsRelative() const
{
	bool result = true;
	if (!m_strFileName.empty())
	{
		//---
		// Look for unix "/"...
		// ESH: Look for Windows "\" (with prepending escape character \)
		//---
		if ( (*(m_strFileName.begin()) == '/') || (*(m_strFileName.begin()) == '\\') )
		{
			result = false;
		}
		else
		{
			// Look for windows drive
			if (m_strFileName.size() < 2)
			{
				return true;
			}

			else
			{
				char cBegin = m_strFileName[0];
				char cSecond = m_strFileName[1];
				if ( ((cBegin >= 'a' && cBegin <= 'z') || (cBegin >= 'A' && cBegin <= 'Z')) && cSecond == ':')
				{
					return false;
				}
			}
		}
	}
	return result;
}

std::string FileName::GetExtention() const
{
	std::string::size_type pos = m_strFileName.rfind('.');
	if (pos == std::string::npos)
	{
		return "";
	}

	std::string strTemp = m_strFileName;

	return (strTemp.substr(pos+1));
}

std::string FileName::GetPath() const
{
	std::string::size_type pos = m_strFileName.rfind(PATHSPLIT);

	if (pos == 0)
		return m_strFileName;
	if (pos == std::string::npos)
	{
		return "";
	}

	std::string strTemp = m_strFileName;

	return (strTemp.substr(0, pos));
}

std::string FileName::GetDrive() const
{
	std::string::size_type pos = m_strFileName.find_first_of(':');
	if (pos == std::string::npos || pos != 1)
	{
		return "";
	}

	std::string strTemp = m_strFileName;
	return (strTemp.substr(0, pos));
}

std::string FileName::GetFile() const
{
	std::string::size_type pos = m_strFileName.rfind(PATHSPLIT);
	if (pos == std::string::npos)
		return m_strFileName;
	else
	{
		std::string strTemp = m_strFileName;
		return (strTemp.substr(pos+1));
	}
}

std::string FileName::GetFileNoExtension() const
{
	std::string strTemp = m_strFileName;

	std::string::size_type dot_pos  = strTemp.rfind('.');
	std::string::size_type slash_pos = strTemp.rfind(PATHSPLIT);

	if(dot_pos == std::string::npos)
	{
		if(slash_pos == std::string::npos)
		{
			return m_strFileName;
		}
		else
		{
			return std::string(strTemp.begin()+slash_pos+1,
				strTemp.end());
		}
	}
	else if(slash_pos == std::string::npos)
	{
		return std::string(strTemp.begin(), strTemp.begin()+dot_pos);
	}
	else if(slash_pos < dot_pos)
	{
		return std::string(strTemp.begin()+slash_pos+1,
			strTemp.begin() + dot_pos);
	}

	return std::string(strTemp.begin()+slash_pos+1,
		strTemp.end());
}

std::string FileName::GetNoExtension() const
{
	std::string::size_type pos = m_strFileName.rfind('.');
	if (pos == std::string::npos)
	{
		return m_strFileName;
	}

	std::string strTemp = m_strFileName;

	return strTemp.substr(0,pos);
}

std::string FileName::SetExtension(const std::string& e)
{
	std::string::size_type pos = m_strFileName.rfind('.');
	if (pos == std::string::npos)
	{
		m_strFileName += ".";
		return m_strFileName += e;
	}

	std::string strTemp = m_strFileName;

	m_strFileName = strTemp.substr(0,pos+1);

	return m_strFileName += e;
}

bool FileName::Remove(const std::string& strPathName)
{
	bool result = true;

	FileName sFile(strPathName);

#if defined(__VISUALC__)  || defined(__BORLANDC__) || defined(__WATCOMC__) || \
	defined(__GNUWIN32__) || defined(_MSC_VER)

	if(sFile.IsDir())
	{
		// Note this only removes empty directories.
		result = ( RemoveDirectory( strPathName.c_str() ) != 0 );
	}
	else
	{
		result = ( DeleteFile( strPathName.c_str() ) != 0 );
	}
#else /* Unix flavor from unistd.h. */
	if(sFile.IsDir())
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

bool FileName::Remove() const
{
	return FileName::Remove(m_strFileName);
}

bool FileName::CreateDirectory(bool bRecurseFlag/*=true*/,int nPerm/*=0775*/) const
{
	if ( m_strFileName.empty() ) return false;

	FileName sFile(m_strFileName);
	if(sFile.Exists()) return true;

	if(bRecurseFlag)
	{
		Expand();
		std::string tempString = m_strFileName;
		vector<std::string> result;

		std::stringstream ss;
		ss<<PATHSPLIT;
		std::string strPathSlpit = ss.str();
		StringUtil::Split(tempString,strPathSlpit,result);

		if(!result.empty())
		{
			std::string current = result[0];

			// Reconstruct UNC paths under Windows.
#if defined(_WIN32)
			bool bGotUNC = false;
			if ( current.length() == 0 && tempString.length() > 2 )
			{
				const char* fstr = tempString.c_str();
				const char fstar0 = fstr[0];
				const char fstar1 = fstr[1];
				if ( fstar0=='\\' && fstar1=='\\' )
				{
					bGotUNC = true;
					current = PATHSPLIT;
				}
			}
#endif

			for(uint32_t i = 1; i < result.size(); ++i)
			{
				current += (PATHSPLIT+result[i]);

#if defined(_WIN32)
				if ( bGotUNC == true && i==1 )
				{
					// The root of the UNC path is assumed to exist.
					continue;
				}
#endif

				if(current != strPathSlpit)
				{
					if(!FileName(current).Exists())
					{
#if defined(__BORLANDC__)
						if ( _mkdir(current.c_str()) != 0 )
#elif defined(_WIN32)
						if ( _mkdir(current.c_str()) != 0 )
#else
						if ( mkdir(current.c_str(), nPerm) != 0 )
#endif
						{
							return false;
						}
					}
				}
			}
		}
	}
	else
	{
#if defined (__BORLANDC__)
		if ( _mkdir(m_strFileName.c_str()) != 0 )
#elif defined(_WIN32)
		if ( _mkdir(m_strFileName.c_str()) != 0 )
#else
		if ( mkdir(m_strFileName.c_str(), nPerm) != 0 )
#endif
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	return true;
}

bool FileName::Rename(const std::string& strDestFile, bool overwriteDestinationFlag/*=true*/) const
{
	FileName sFile(strDestFile);
	if(!overwriteDestinationFlag)
	{
		if(sFile.Exists())
		{
			cout<< "\nDestination File Exists: " << strDestFile << std::endl;
			return false;
		}
	}

#if defined(_WIN32)
	const DWORD flags = overwriteDestinationFlag ? MOVEFILE_REPLACE_EXISTING : 0;
	return MoveFileExA(m_strFileName.c_str(), strDestFile.c_str(), flags) != 0;
#else
	return ::rename(m_strFileName.c_str(), strDestFile.c_str()) == 0;
#endif
}

void FileName::Split(std::string& drivePart, std::string& pathPart, std::string& filePart, std::string& extPart) const
{
	drivePart.clear();
	pathPart.clear();
	filePart.clear();
	extPart.clear();

	size_t pathBegin = 0;
#if defined(_WIN32)
	if (m_strFileName.size() >= 2 && m_strFileName[1] == ':')
	{
		drivePart = m_strFileName.substr(0, 2);
		pathBegin = 2;
	}
#endif

	const size_t separator = m_strFileName.find_last_of("/\\");
	if (separator != std::string::npos && separator >= pathBegin)
	{
		pathPart = m_strFileName.substr(pathBegin, separator - pathBegin + 1);
	}

	const size_t fileBegin = separator == std::string::npos ? pathBegin : separator + 1;
	const std::string wholeName = m_strFileName.substr(fileBegin);
	const size_t dot = wholeName.find_last_of('.');
	if (dot == std::string::npos)
	{
		filePart = wholeName;
	}
	else
	{
		filePart = wholeName.substr(0, dot);
		extPart = wholeName.substr(dot);
	}
}

std::string FileName::GetFileName() const
{
	return m_strFileName;
}

void FileName::SetFileName(const std::string& strFileName)
{
	m_strFileName = strFileName;
}

FileName FileName::Expand() const
{
	if (m_strFileName.empty())
		return FileName();

	std::string expanded = m_strFileName;
	if (expanded.size() >= 2 && expanded[0] == '~' &&
		(expanded[1] == '/' || expanded[1] == '\\'))
	{
		const std::string home = EnvironmentUtility::GetInstance().GetUserDir();
		if (!home.empty())
			expanded = (fs::path(home) / expanded.substr(2)).string();
	}

	// 支持原接口文档中的 $(NAME) 环境变量形式。
	size_t begin = 0;
	while ((begin = expanded.find("$(", begin)) != std::string::npos)
	{
		const size_t end = expanded.find(')', begin + 2);
		if (end == std::string::npos)
			break;
		const std::string name = expanded.substr(begin + 2, end - begin - 2);
		const std::string value = EnvironmentUtility::GetInstance().GetEnvironmentVariable(name);
		expanded.replace(begin, end - begin + 1, value);
		begin += value.size();
	}

	std::error_code ec;
	fs::path path(expanded);
	if (path.is_relative())
		path = fs::absolute(path, ec);
	if (ec)
		return *this;

	return FileName(path.lexically_normal().string());
}

bool FileName::IsNeedExpansion() const
{
	bool result = false;
	if ( !m_strFileName.empty() )
	{
		result = IsRelative();
		if (result == false)		//如果不是相对路径，需要检查'$'是否存在
		{
			// Check for '$'
			std::string::size_type pos = m_strFileName.find('$', 0);
			if (pos != std::string::npos)
			{
				// found '$'
				result = true;
			}
		}    
	}
	return result;
}

NS_BASELIB_END

