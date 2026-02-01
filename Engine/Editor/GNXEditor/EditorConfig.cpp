//
//  EditorConfig.cpp
//  GNXEngine
//

#include "EditorConfig.h"
#include <QStandardPaths>
#include <QDir>
#include <fstream>
#include <sstream>
#include <algorithm>

// 配置文件名
const char* EditorConfig::CONFIG_FILE_NAME = "GNXEngine_EditorConfig.json";

EditorConfig::EditorConfig()
{
}

EditorConfig::~EditorConfig()
{
    SaveConfig();
}

EditorConfig& EditorConfig::GetInstance()
{
    static EditorConfig instance;
    return instance;
}

std::string EditorConfig::GetConfigFilePath() const
{
    // 使用 QStandardPaths 获取应用数据目录
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(configDir);

    // 确保目录存在
    if (!dir.exists())
    {
        dir.mkpath(".");
    }

    return (fs::path(configDir.toStdString()) / CONFIG_FILE_NAME).string();
}

bool EditorConfig::LoadConfig()
{
    std::string configPath = GetConfigFilePath();
    std::ifstream file(configPath);

    if (!file.is_open())
    {
        // 配置文件不存在，使用默认值
        return false;
    }

    try
    {
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string jsonStr = buffer.str();
        file.close();

        // 解析 JSON（这里使用简单的字符串解析，避免引入 nlohmann/json）
        // TODO: 使用 JSON 库替代
        size_t pos = 0;

        // 解析 lastOpenProjectPath
        pos = jsonStr.find("\"lastOpenProjectPath\"");
        if (pos != std::string::npos)
        {
            size_t start = jsonStr.find("\"", pos + 20) + 1;
            size_t end = jsonStr.find("\"", start);
            mLastOpenProjectPath = jsonStr.substr(start, end - start);
        }

        // 解析 recentProjects
        pos = jsonStr.find("\"recentProjects\"");
        if (pos != std::string::npos)
        {
            mRecentProjects.clear();
            size_t start = jsonStr.find("[", pos);
            size_t end = jsonStr.find("]", start);
            std::string array = jsonStr.substr(start + 1, end - start - 1);

            size_t itemStart = 0;
            size_t itemEnd = 0;
            while ((itemStart = array.find("\"", itemEnd)) != std::string::npos)
            {
                itemStart += 1;
                itemEnd = array.find("\"", itemStart);
                if (itemEnd == std::string::npos) break;

                std::string path = array.substr(itemStart, itemEnd - itemStart);
                mRecentProjects.push_back(path);
                itemEnd += 1;
            }
        }

        return true;
    }
    catch (const std::exception& e)
    {
        // 解析失败，使用默认值
        return false;
    }
}

bool EditorConfig::SaveConfig()
{
    std::string configPath = GetConfigFilePath();

    try
    {
        std::ofstream file(configPath);
        if (!file.is_open())
        {
            return false;
        }

        // 构建 JSON 字符串（手动构建，避免引入 JSON 库）
        file << "{\n";
        file << "  \"lastOpenProjectPath\": \"" << mLastOpenProjectPath << "\",\n";

        // 保存最近工程列表
        file << "  \"recentProjects\": [\n";
        for (size_t i = 0; i < mRecentProjects.size(); ++i)
        {
            file << "    \"" << mRecentProjects[i] << "\"";
            if (i < mRecentProjects.size() - 1)
            {
                file << ",";
            }
            file << "\n";
        }
        file << "  ],\n";

        // 保存窗口设置
        file << "  \"window\": {\n";
        file << "    \"width\": " << mWindowWidth << ",\n";
        file << "    \"height\": " << mWindowHeight << ",\n";
        file << "    \"x\": " << mWindowX << ",\n";
        file << "    \"y\": " << mWindowY << "\n";
        file << "  }\n";

        file << "}\n";
        file.close();

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

void EditorConfig::AddRecentProject(const std::string& projectPath)
{
    // 如果已经存在，先移除
    auto it = std::find(mRecentProjects.begin(), mRecentProjects.end(), projectPath);
    if (it != mRecentProjects.end())
    {
        mRecentProjects.erase(it);
    }

    // 添加到列表开头
    mRecentProjects.insert(mRecentProjects.begin(), projectPath);

    // 限制数量
    if (mRecentProjects.size() > MAX_RECENT_PROJECTS)
    {
        mRecentProjects.resize(MAX_RECENT_PROJECTS);
    }

    // 保存配置
    SaveConfig();
}

std::vector<std::string> EditorConfig::GetRecentProjects() const
{
    return mRecentProjects;
}

void EditorConfig::ClearRecentProjects()
{
    mRecentProjects.clear();
    SaveConfig();
}
