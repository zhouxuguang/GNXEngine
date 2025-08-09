//
//  FileUtil.h 文件操作类跨平台工具类
//  BaseLib
//
//  Created by Zhou,Xuguang on 17/5/12.
//  Copyright © 2017年 Zhou,Xuguang. All rights reserved.
//

#ifndef BASELIB_FILEUTIL_INCLUDE_H_FFEEGMKDMS6WQFWQ2432FD
#define BASELIB_FILEUTIL_INCLUDE_H_FFEEGMKDMS6WQFWQ2432FD

#include "PreCompile.h"

NS_BASELIB_BEGIN

class BASELIB_API FileUtil
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
    
    //文件是否存在
    static bool IsExist(const std::string& strFileName);
    
    //判断路径是否是文件
    static bool IsFile(const std::string& strFileName);
    
    //判断路径是否是目录
    static bool IsDir(const std::string& strFileName);
    
    //是否可读
    static bool IsReadable(const std::string &strFileName);
    
    //是否可写
    static bool IsWriteable(const std::string &strFileName);
    
    //是否可执行
    static bool IsExecutable(const std::string &strFileName);
    
    //获得文件大小
    static int64_t GetFileSize(const std::string &strFileName);
    
    /**
     * @brief 是否相对路径
     *
     * @return true if file name has a relative path, false if absolute.
     *
     * @note This will return true if file name is empty.
     */
    static bool IsRelative(const std::string &strFileName);
    
    //移除文件或者目录
    static bool Remove(const std::string& strPathName);
    
    /**
     * 创建目录，递归的创建
     * 
     */
    static bool MakeDirectory(const std::string& directoryPath);

    /**
     * 读取整个二进制文件到字节向量
     *
     * @param path 文件路径
     * @return 包含文件内容的字节向量，如果出错则返回空向量
     */
    static std::vector<uint8_t> ReadBinaryFile(const std::string& path);

    /**
     * 将原始内存数据写入二进制文件
     *
     * @param path 文件路径
     * @param data 指向数据的指针
     * @param size 数据字节数
     * @return 写入成功返回true，否则false
     */
    static bool WriteBinaryFile(const std::string& path, const void* data, size_t size);

    /**
     * 将字节向量写入二进制文件
     *
     * @param path 文件路径
     * @param data 包含数据的字节向量
     * @return 写入成功返回true，否则false
     */
    static bool WriteBinaryFile(const std::string& path, const std::vector<uint8_t>& data);
    
private:
    FileUtil();
    ~FileUtil();
    FileUtil(const FileUtil& );
    FileUtil& operator = (const FileUtil&);
};

NS_BASELIB_END

#endif /* BASELIB_FILEUTIL_INCLUDE_H_FFEEGMKDMS6WQFWQ2432FD */
