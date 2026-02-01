//
//  EditorConfig.h
//  GNXEngine
//
//  编辑器配置管理（编辑器级别，不是工程级别）
//

#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

class EditorConfig
{
public:
    static EditorConfig& GetInstance();

    // 加载配置
    bool LoadConfig();

    // 保存配置
    bool SaveConfig();

    // 获取配置文件路径
    std::string GetConfigFilePath() const;

    // 上次打开工程时的路径
    void SetLastOpenProjectPath(const std::string& path) { mLastOpenProjectPath = path; }
    std::string GetLastOpenProjectPath() const { return mLastOpenProjectPath; }

    // 最近工程列表
    void AddRecentProject(const std::string& projectPath);
    std::vector<std::string> GetRecentProjects() const;
    void ClearRecentProjects();

    // 窗口大小和位置
    void SetWindowSize(int width, int height) { mWindowWidth = width; mWindowHeight = height; }
    void GetWindowSize(int& width, int& height) const { width = mWindowWidth; height = mWindowHeight; }

    void SetWindowPosition(int x, int y) { mWindowX = x; mWindowY = y; }
    void GetWindowPosition(int& x, int& y) const { x = mWindowX; y = mWindowY; }

private:
    EditorConfig();
    ~EditorConfig();

    // 配置文件名
    static const char* CONFIG_FILE_NAME;

    // 上次打开工程时的路径（用于 QFileDialog 的初始路径）
    std::string mLastOpenProjectPath;

    // 最近工程列表（最多保存10个）
    std::vector<std::string> mRecentProjects;
    static const size_t MAX_RECENT_PROJECTS = 10;

    // 窗口设置
    int mWindowWidth = 1280;
    int mWindowHeight = 720;
    int mWindowX = 100;
    int mWindowY = 100;
};
