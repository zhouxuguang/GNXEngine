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
    struct stat sbuf;
    
    //还有点问题
    
#ifndef __BORLANDC__
    if (stat(strFileName.c_str(), &sbuf) == 0)
    {
        return (int64_t)sbuf.st_size;
    }
    else
    {
//        std::ifstream in(strFileName.c_str());
//        if(in)
//        {
//            in.seekg(SEEK_END);
//            return (int64_t)in.tellg();
//        }
    }
#else
    ifstream in(strFileName.c_str());
    if(in)
    {
        in.seekg(SEEK_END);
        return (int64_t)in.tellg();
    }
#endif
    return 0;
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
    
//    struct stat64 stbuf;
//#ifdef OSG_USE_UTF8_FILENAME
//    if( _wstat64( OSGDB_STRING_TO_FILENAME(path).c_str(), &stbuf ) == 0 )
//#else
//        if( stat64( path.c_str(), &stbuf ) == 0 )
//#endif
//        {
//            if( S_ISDIR(stbuf.st_mode))
//                return true;
//            else
//            {
//                OSG_DEBUG << "osgDB::makeDirectory(): "  <<
//                path << " already exists and is not a directory!" << std::endl;
//                return false;
//            }
//        }
//
//    std::string dir = path;
//    std::stack<std::string> paths;
//    while( true )
//    {
//        if( dir.empty() )
//            break;
//
//#ifdef OSG_USE_UTF8_FILENAME
//        if( _wstat64( OSGDB_STRING_TO_FILENAME(dir).c_str(), &stbuf ) < 0 )
//#else
//            if( stat64( dir.c_str(), &stbuf ) < 0 )
//#endif
//            {
//                switch( errno )
//                {
//                    case ENOENT:
//                    case ENOTDIR:
//                        paths.push( dir );
//                        break;
//
//                    default:
//                        OSG_DEBUG << "osgDB::makeDirectory(): "  << strerror(errno) << std::endl;
//                        return false;
//                }
//            }
//        dir = osgDB::getFilePath(std::string(dir));
//    }
//
//    while( !paths.empty() )
//    {
//        std::string dir = paths.top();
//
//#if defined(WIN32)
//        //catch drive name
//        if (dir.size() == 2 && dir.c_str()[1] == ':') {
//            paths.pop();
//            continue;
//        }
//#endif
//
//#ifdef OSG_USE_UTF8_FILENAME
//        if ( _wmkdir(OSGDB_STRING_TO_FILENAME(dir).c_str())< 0 )
//#else
//            if( mkdir( dir.c_str(), 0755 )< 0 )
//#endif
//            {
//                if (osgDB::fileExists(dir))
//                {
//                    OE_DEBUG << "Attempt to create directory that already exists " << dir << std::endl;
//                }
//                else
//                {
//                    OSG_DEBUG << "osgDB::makeDirectory(): "  << strerror(errno) << std::endl;
//                    return false;
//                }
//            }
//        paths.pop();
//    }
//    return true;
    
    return false;
}

NS_BASELIB_END
