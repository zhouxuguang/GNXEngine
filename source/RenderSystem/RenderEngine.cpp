//
//  RenderEngine.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/29.
//

#include "RenderEngine.h"

#include <iostream>
#include <string>
#include <stdlib.h>     // 用于 getenv 函数
#include <unistd.h>     // 用于 readlink 函数

std::string GetCurrentWorkingDirectory()
{
    std::string cwd;
    const uint32_t kMaxPathLength = 1024;
    char path[kMaxPathLength];
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    // Windows 平台上使用 GetModuleFileNameA 获取可执行文件路径
    DWORD result = GetModuleFileNameA(nullptr, path, kMaxPathLength);
    if (result == 0 || result == kMaxPathLength) {
        return "";
    }
#else
    // macOS 上使用 readlink 获取可执行文件路径
    ssize_t result = readlink("/proc/self/exe", path, kMaxPathLength - 1);
    if (result == -1) {
        return "";
    }
    path[result] = '\0';
#endif

    // 从可执行文件路径中解析出当前源文件所在的目录
    char* pos = strrchr(path, '/');
    if (pos == nullptr) {
        return "";
    }
    *pos = '\0';
    cwd = path;
    return cwd;
}

//  /Users/zhouxuguang/work/mycode/GNXEngine/source/shader/built-in

std::string getBuiltInShaderDir()
{
    std::string path = __FILE__;
    path = path.substr(0, path.find_last_of("/"));
    return path + "/../shader/built-in/";
}

// /Users/zhouxuguang/work/mycode/GNXEngine/GNXEditor/media

std::string getMediaDir()
{
    std::string path = __FILE__;
    path = path.substr(0, path.find_last_of("/"));
    return path + "/../../GNXEditor/media/";
}
