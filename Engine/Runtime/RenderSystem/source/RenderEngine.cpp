//
//  RenderEngine.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/29.
//

#include "RenderEngine.h"
#include "Runtime/BaseLib/include/BaseLib.h"

#include <iostream>
#include <string>
#include <stdlib.h>     // 用于 getenv 函数

#ifndef _WIN32
#include <unistd.h>     // 用于 readlink 函数
#else
#include <Windows.h>
#endif // !_WIN32

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
    /*std::string path = __FILE__;
    path = path.substr(0, path.find_last_of(PATHSPLIT));
    std::string pathSplit = std::string(1, PATHSPLIT);
    return path + pathSplit + ".." + pathSplit + "shader" + pathSplit + "built-in" + pathSplit;*/

	fs::path path = __FILE__;
	fs::path parentDir = path.parent_path();
	path = (parentDir / fs::path("../../../Shader/built-in/")).lexically_normal();
	return path.string();

    //return path + R"(../shader/built-in/)";
}

// /Users/zhouxuguang/work/mycode/GNXEngine/GNXEditor/media

std::string getMediaDir()
{
    fs::path path = __FILE__;
    fs::path parentDir = path.parent_path();
    path = (parentDir / fs::path("../../../Editor/media/")).lexically_normal();
    return path.string();
    /*path = path.substr(0, path.find_last_of(PATHSPLIT));
    std::string pathSplit = std::string(1, PATHSPLIT);
    return path + pathSplit + ".." + pathSplit + ".." + pathSplit + "GNXEditor" + pathSplit + "media" + pathSplit;*/
    //return path + R"(/../../GNXEditor/media/)";
}

std::string getAssetsDir()
{
	fs::path path = __FILE__;
	fs::path parentDir = path.parent_path();
	path = (parentDir / fs::path("../../../Editor/Assets/")).lexically_normal();
	return path.string();
}

std::string GetProjectAssetDir()
{
	fs::path path = __FILE__;
	fs::path parentDir = path.parent_path();
	path = (parentDir / fs::path("../../../../data_asset/")).lexically_normal();
	return path.string();
}

bool EnsurePathExists(const fs::path& path) 
{
	if (fs::exists(path)) 
    {
		std::cout << "路径已存在: " << path << std::endl;
		return true;
	}
	else 
    {
		try 
        {
			// 创建所有不存在的父目录
			bool success = fs::create_directories(path);
			if (success) 
            {
				std::cout << "成功创建路径: " << path << std::endl;
				return true;
			}
			else 
            {
				std::cerr << "创建路径失败: " << path << std::endl;
				return false;
			}
		}
		catch (const fs::filesystem_error& e) 
        {
			std::cerr << "文件系统错误: " << e.what() << std::endl;
			return false;
		}
	}
}
