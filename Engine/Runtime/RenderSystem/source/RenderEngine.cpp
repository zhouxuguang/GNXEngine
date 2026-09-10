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
#include <cstdlib>

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

std::string GetCurrentWorkingDirectory()
{
#if defined(_WIN32)
    std::wstring path(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, path.data(),
                                            static_cast<DWORD>(path.size()));
    if (length == 0 || length == path.size())
        return {};
    path.resize(length);
    return fs::path(path).parent_path().string();
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string path(size, '\0');
    if (_NSGetExecutablePath(path.data(), &size) != 0)
        return {};
    return fs::weakly_canonical(fs::path(path.c_str())).parent_path().string();
#else
    std::string path(4096, '\0');
    const ssize_t result = readlink("/proc/self/exe", path.data(), path.size() - 1);
    if (result < 0)
        return {};
    path[result] = '\0';
    return fs::path(path.c_str()).parent_path().string();
#endif
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

std::string getCompiledShaderDir()
{
    return (fs::path(GetProjectAssetDir()) / "Shader").string() + PATHSPLIT;
}

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
	auto withSeparator = [](const fs::path& path)
	{
		std::string value = path.lexically_normal().string();
		if (!value.empty() && value.back() != '/' && value.back() != '\\')
			value += PATHSPLIT;
		return value;
	};
	if (const char* configured = std::getenv("GNX_ENGINE_CONTENT_ROOT"))
	{
		const fs::path path(configured);
		if (fs::is_directory(path))
			return withSeparator(path);
	}
	const fs::path deployed = fs::path(GetCurrentWorkingDirectory()) / "data_asset";
	if (fs::is_directory(deployed))
		return withSeparator(deployed);
	const fs::path source =
		(fs::path(__FILE__).parent_path() / "../../../../data_asset").lexically_normal();
	return withSeparator(source);
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
