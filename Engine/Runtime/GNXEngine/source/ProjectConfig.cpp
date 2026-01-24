//
//  ProjectConfig.cpp
//  GNXEngine
//

#include "ProjectConfig.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>

using json = nlohmann::json;

NAMESPACE_GNXENGINE_BEGIN

// JSON 序列化
std::string ProjectConfig::ToJson() const
{
    json j;

    j["projectName"] = projectName;
    j["version"] = version;
    j["creationDate"] = creationDate;
    j["lastModified"] = lastModified;
    j["renderPath"] = static_cast<int>(renderPath);
    j["defaultWidth"] = defaultWidth;
    j["defaultHeight"] = defaultHeight;
    j["vsyncEnabled"] = vsyncEnabled;
    j["defaultScene"] = defaultScene;
    j["recentScenes"] = recentScenes;

    return j.dump(4); // 4个空格缩进，格式化输出
}

// JSON 反序列化
bool ProjectConfig::FromJson(const std::string& jsonStr)
{
    try
    {
        json j = json::parse(jsonStr);

        projectName = j.value("projectName", "");
        version = j.value("version", "1.0.0");
        creationDate = j.value("creationDate", "");
        lastModified = j.value("lastModified", "");
        renderPath = static_cast<RenderPath>(j.value("renderPath", 1));
        defaultWidth = j.value("defaultWidth", 1280);
        defaultHeight = j.value("defaultHeight", 720);
        vsyncEnabled = j.value("vsyncEnabled", true);
        defaultScene = j.value("defaultScene", "");

        if (j.contains("recentScenes") && j["recentScenes"].is_array())
        {
            recentScenes = j["recentScenes"].get<std::vector<std::string>>();
        }

        return true;
    }
    catch (const json::exception& e)
    {
        LOG_ERROR("Failed to parse JSON: %s", e.what());
        return false;
    }
}

bool ProjectConfig::SaveToFile(const std::string& filepath)
{
    std::ofstream file(filepath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to save project file: %s", filepath.c_str());
        return false;
    }

    std::string json = ToJson();
    file << json;
    file.close();

    LOG_INFO("Project saved to: %s", filepath.c_str());
    return true;
}

bool ProjectConfig::LoadFromFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to load project file: %s", filepath.c_str());
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jsonStr = buffer.str();
    file.close();

    bool success = FromJson(jsonStr);
    if (success)
    {
        // 设置工程路径
        fs::path path = filepath;
        projectPath = path.parent_path().string();

        // 设置子目录路径
        assetsPath = (path.parent_path() / "Assets").string();
        scenesPath = (path.parent_path() / "Scenes").string();
        settingsPath = (path.parent_path() / "Settings").string();

        LOG_INFO("Project loaded: %s", projectName.c_str());
        LOG_INFO("Project path: %s", projectPath.c_str());
    }

    return success;
}

std::string ProjectConfig::GetProjectDirectory() const
{
    return projectPath;
}

std::string ProjectConfig::GetAbsoluteAssetPath(const std::string& relativePath) const
{
    return (fs::path(assetsPath) / relativePath).string();
}

std::string ProjectConfig::GetAbsoluteScenePath(const std::string& relativePath) const
{
    return (fs::path(scenesPath) / relativePath).string();
}

std::string ProjectConfig::GetRelativeAssetPath(const std::string& absolutePath) const
{
    fs::path absPath(absolutePath);
    fs::path assetDir(assetsPath);
    if (absPath.is_absolute() && absPath.string().find(assetDir.string()) == 0)
    {
        return fs::relative(absPath, assetDir).string();
    }
    return absolutePath;
}

std::string ProjectConfig::GetRelativeScenePath(const std::string& absolutePath) const
{
    fs::path absPath(absolutePath);
    fs::path sceneDir(scenesPath);
    if (absPath.is_absolute() && absPath.string().find(sceneDir.string()) == 0)
    {
        return fs::relative(absPath, sceneDir).string();
    }
    return absolutePath;
}

// ========== ProjectManager ==========

ProjectManager::ProjectManager()
{
}

ProjectManager::~ProjectManager()
{
    CloseProject();
}

ProjectManager& ProjectManager::GetInstance()
{
    static ProjectManager instance;
    return instance;
}

bool ProjectManager::CreateNewProject(const std::string& projectPath, const std::string& projectName)
{
    // 检查路径是否存在
    if (fs::exists(projectPath))
    {
        LOG_ERROR("Project path already exists: %s", projectPath.c_str());
        return false;
    }

    // 创建工程目录
    if (!fs::create_directories(projectPath))
    {
        LOG_ERROR("Failed to create project directory: %s", projectPath.c_str());
        return false;
    }

    // 创建默认目录结构
    if (!CreateProjectDirectories(projectPath))
    {
        LOG_ERROR("Failed to create project directories");
        return false;
    }

    // 创建工程配置
    ProjectConfig* config = new ProjectConfig();
    config->projectName = projectName;
    config->version = "1.0.0";

    // 获取当前时间
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    config->creationDate = oss.str();
    config->lastModified = config->creationDate;

    // 设置工程路径
    config->projectPath = projectPath;
    config->assetsPath = (fs::path(projectPath) / "Assets").string();
    config->scenesPath = (fs::path(projectPath) / "Scenes").string();
    config->settingsPath = (fs::path(projectPath) / "Settings").string();

    // 保存工程文件
    std::string projectFile = (fs::path(projectPath) / (projectName + ".gnxproj")).string();
    if (!config->SaveToFile(projectFile))
    {
        delete config;
        return false;
    }

    // 关闭旧工程（如果有）
    CloseProject();

    // 设置当前工程
    mProject = config;

    // 添加到最近工程列表
    AddRecentProject(projectFile);

    LOG_INFO("Project created: %s", projectName.c_str());
    return true;
}

bool ProjectManager::OpenProject(const std::string& projectPath)
{
    // 检查文件是否存在
    if (!fs::exists(projectPath))
    {
        LOG_ERROR("Project file not found: %s", projectPath.c_str());
        return false;
    }

    // 关闭旧工程（如果有）
    CloseProject();

    // 加载工程配置
    ProjectConfig* config = new ProjectConfig();
    if (!config->LoadFromFile(projectPath))
    {
        delete config;
        return false;
    }

    // 设置当前工程
    mProject = config;

    // 添加到最近工程列表
    AddRecentProject(projectPath);

    LOG_INFO("Project opened: %s", config->projectName.c_str());
    return true;
}

bool ProjectManager::CloseProject()
{
    if (mProject)
    {
        // 保存工程配置
        std::string projectFile = (fs::path(mProject->projectPath) / (mProject->projectName + ".gnxproj")).string();
        mProject->SaveToFile(projectFile);

        delete mProject;
        mProject = nullptr;

        LOG_INFO("Project closed");
    }

    return true;
}

bool ProjectManager::SaveProject()
{
    if (!mProject)
    {
        LOG_WARN("No project open");
        return false;
    }

    // 更新最后修改时间
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    mProject->lastModified = oss.str();

    // 保存工程文件
    std::string projectFile = (fs::path(mProject->projectPath) / (mProject->projectName + ".gnxproj")).string();
    return mProject->SaveToFile(projectFile);
}

std::string ProjectManager::GetProjectPath() const
{
    if (mProject)
    {
        return mProject->projectPath;
    }
    return "";
}

void ProjectManager::AddRecentProject(const std::string& projectPath)
{
    // TODO: 实现最近工程列表的持久化存储
}

std::vector<std::string> ProjectManager::GetRecentProjects() const
{
    // TODO: 从配置文件加载最近工程列表
    return {};
}

bool ProjectManager::CreateProjectDirectories(const std::string& projectPath)
{
    try
    {
        // 创建 Assets 目录（空目录，不创建子目录）
        fs::path assetsDir = fs::path(projectPath) / "Assets";
        fs::create_directories(assetsDir);

        // 创建 Settings 目录
        fs::path settingsDir = fs::path(projectPath) / "Settings";
        fs::create_directories(settingsDir);

        LOG_INFO("Project directories created");
        return true;
    }
    catch (const fs::filesystem_error& e)
    {
        LOG_ERROR("Failed to create directories: %s", e.what());
        return false;
    }
}

bool ProjectManager::IsValidProject(const std::string& projectPath) const
{
    return fs::exists(projectPath) && projectPath.find(".gnxproj") != std::string::npos;
}

NAMESPACE_GNXENGINE_END
