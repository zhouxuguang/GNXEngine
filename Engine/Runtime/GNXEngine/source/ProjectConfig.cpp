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
#include <filesystem>
#include <cstdlib>
#include <cctype>
#include <algorithm>
#include <cwctype>
#if defined(_WIN32)
#include <Windows.h>
#endif

using json = nlohmann::json;

NAMESPACE_GNXENGINE_BEGIN

namespace
{
bool IsWithinProject(const fs::path& child, const fs::path& root)
{
    const fs::path normalizedChild = fs::absolute(child).lexically_normal();
    const fs::path normalizedRoot = fs::absolute(root).lexically_normal();
    auto childIt = normalizedChild.begin();
    for (auto rootIt = normalizedRoot.begin(); rootIt != normalizedRoot.end(); ++rootIt, ++childIt)
    {
        if (childIt == normalizedChild.end())
            return false;
#if defined(_WIN32)
        std::wstring left = childIt->wstring();
        std::wstring right = rootIt->wstring();
        std::transform(left.begin(), left.end(), left.begin(), ::towlower);
        std::transform(right.begin(), right.end(), right.begin(), ::towlower);
        if (left != right)
            return false;
#else
        if (*childIt != *rootIt)
            return false;
#endif
    }
    return true;
}

fs::path RecentProjectsPath()
{
#if defined(_WIN32)
    if (const char* localAppData = std::getenv("LOCALAPPDATA"))
        return fs::path(localAppData) / "GNXEngine" / "RecentProjects.txt";
#else
    if (const char* home = std::getenv("HOME"))
        return fs::path(home) / ".config" / "GNXEngine" / "RecentProjects.txt";
#endif
    return fs::temp_directory_path() / "GNXEngine" / "RecentProjects.txt";
}

void WriteRecentProjects(const std::vector<std::string>& projects)
{
    const fs::path target = RecentProjectsPath();
    std::error_code error;
    fs::create_directories(target.parent_path(), error);
    if (error)
        return;
    const fs::path temporary = target.string() + ".tmp";
    std::ofstream output(temporary, std::ios::trunc);
    for (const std::string& value : projects)
        output << value << '\n';
    output.close();
    if (!output)
        return;
#if defined(_WIN32)
    if (!MoveFileExW(temporary.c_str(), target.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        fs::remove(temporary, error);
#else
    fs::rename(temporary, target, error);
    if (error)
        fs::remove(temporary, error);
#endif
}
}

// JSON 序列化
std::string ProjectConfig::ToJson() const
{
    json j;

    j["schemaVersion"] = schemaVersion;
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
    const fs::path root(projectPath);
    auto relativePath = [&root](const std::string& path, const char* fallback)
    {
        if (path.empty())
            return std::string(fallback);
        std::error_code error;
        fs::path relative = fs::relative(fs::path(path), root, error);
        return error ? std::string(fallback) : relative.generic_string();
    };
    j["paths"] = {
        {"assets", relativePath(assetsPath, "Assets")},
        {"scenes", relativePath(scenesPath, "Scenes")},
        {"settings", relativePath(settingsPath, "Settings")},
        {"cache", relativePath(cachePath, ".gnx/Cache")}
    };

    return j.dump(4); // 4个空格缩进，格式化输出
}

// JSON 反序列化
bool ProjectConfig::FromJson(const std::string& jsonStr)
{
    try
    {
        json j = json::parse(jsonStr);

        schemaVersion = j.value("schemaVersion", 0u);
        if (schemaVersion > 1)
            return false;
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

        const json paths = j.value("paths", json::object());
        auto resolvePath = [this, &paths](const char* key, const char* fallback)
        {
            fs::path value(paths.value(key, fallback));
            if (value.is_relative())
                value = fs::path(projectPath) / value;
            return value.lexically_normal().string();
        };
        assetsPath = resolvePath("assets", "Assets");
        scenesPath = resolvePath("scenes", "Scenes");
        settingsPath = resolvePath("settings", "Settings");
        cachePath = resolvePath("cache", ".gnx/Cache");

        schemaVersion = 1;
        return Validate();
    }
    catch (const json::exception& e)
    {
        LOG_ERROR("Failed to parse JSON: %s", e.what());
        return false;
    }
}

bool ProjectConfig::SaveToFile(const std::string& filepath)
{
    const fs::path target(filepath);
    const fs::path temporary = target.string() + ".tmp";
    std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to save project file: %s", filepath.c_str());
        return false;
    }

    std::string json = ToJson();
    file << json;
    file.flush();
    if (!file.good())
        return false;
    file.close();

    std::error_code error;
#if defined(_WIN32)
    if (!MoveFileExW(temporary.c_str(), target.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        fs::remove(temporary, error);
        return false;
    }
#else
    fs::rename(temporary, target, error);
    if (error)
    {
        fs::remove(temporary, error);
        return false;
    }
#endif
    projectFilePath = target.lexically_normal().string();

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

    fs::path path = fs::absolute(filepath).lexically_normal();
    projectPath = path.parent_path().string();
    projectFilePath = path.string();
    bool success = FromJson(jsonStr);
    if (success)
    {
        LOG_INFO("Project loaded: %s", projectName.c_str());
        LOG_INFO("Project path: %s", projectPath.c_str());
    }

    return success;
}

bool ProjectConfig::Validate(std::string* errorMessage) const
{
    auto fail = [errorMessage](const char* message)
    {
        if (errorMessage)
            *errorMessage = message;
        return false;
    };
    if (projectName.empty())
        return fail("Project name is empty");
    if (projectPath.empty() || assetsPath.empty() || scenesPath.empty() ||
        settingsPath.empty() || cachePath.empty())
        return fail("Project paths are incomplete");
    const fs::path root(projectPath);
    if (!fs::is_directory(root) || !fs::is_directory(assetsPath) ||
        !fs::is_directory(scenesPath) || !fs::is_directory(settingsPath) ||
        !fs::is_directory(cachePath))
        return fail("Project directories are missing");
    if (!IsWithinProject(assetsPath, root) || !IsWithinProject(scenesPath, root) ||
        !IsWithinProject(settingsPath, root) || !IsWithinProject(cachePath, root))
        return fail("Project paths must stay inside the project directory");
    if (defaultWidth == 0 || defaultHeight == 0 ||
        defaultWidth > 16384 || defaultHeight > 16384)
        return fail("Invalid default viewport size");
    if (renderPath != RenderPath::Forward && renderPath != RenderPath::Deferred)
        return fail("Invalid render path");
    return true;
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

std::string ProjectConfig::GetAbsoluteCachePath(const std::string& relativePath) const
{
    return (fs::path(cachePath) / relativePath).string();
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
    if (mProject)
    {
        SaveProject();
        delete mProject;
        mProject = nullptr;
    }
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
        std::error_code error;
        fs::remove_all(projectPath, error);
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
    config->projectFilePath = (fs::path(projectPath) / (projectName + ".gnxproj")).string();
    config->assetsPath = (fs::path(projectPath) / "Assets").string();
    config->scenesPath = (fs::path(projectPath) / "Scenes").string();
    config->settingsPath = (fs::path(projectPath) / "Settings").string();
    config->cachePath = (fs::path(projectPath) / ".gnx" / "Cache").string();

    // 保存工程文件
    std::string projectFile = config->projectFilePath;
    if (!config->SaveToFile(projectFile))
    {
        delete config;
        std::error_code error;
        fs::remove_all(projectPath, error);
        return false;
    }

    if (mProject && !CloseProject())
    {
        delete config;
        std::error_code error;
        fs::remove_all(projectPath, error);
        return false;
    }

    // 设置当前工程
    mProject = config;

    // 添加到最近工程列表
    AddRecentProject(projectFile);

    LOG_INFO("Project created: %s", projectName.c_str());
    return true;
}

bool ProjectManager::OpenProject(const std::string& projectPath)
{
    if (!IsValidProject(projectPath))
    {
        LOG_ERROR("Invalid project file: %s", projectPath.c_str());
        return false;
    }

    ProjectConfig* config = new ProjectConfig();
    if (!config->LoadFromFile(projectPath))
    {
        delete config;
        return false;
    }

    if (mProject && !CloseProject())
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
        const std::string projectFile = mProject->projectFilePath.empty()
            ? (fs::path(mProject->projectPath) / (mProject->projectName + ".gnxproj")).string()
            : mProject->projectFilePath;
        if (!mProject->SaveToFile(projectFile))
            return false;

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
    const std::string projectFile = mProject->projectFilePath.empty()
        ? (fs::path(mProject->projectPath) / (mProject->projectName + ".gnxproj")).string()
        : mProject->projectFilePath;
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
    std::vector<std::string> projects = GetRecentProjects();
    const std::string normalized = fs::absolute(projectPath).lexically_normal().string();
    projects.erase(std::remove_if(projects.begin(), projects.end(),
        [&normalized](const std::string& value) {
#if defined(_WIN32)
            return _stricmp(value.c_str(), normalized.c_str()) == 0;
#else
            return value == normalized;
#endif
        }), projects.end());
    if (fs::is_regular_file(normalized))
        projects.insert(projects.begin(), normalized);
    if (projects.size() > 10)
        projects.resize(10);
    WriteRecentProjects(projects);
}

std::vector<std::string> ProjectManager::GetRecentProjects() const
{
    std::vector<std::string> projects;
    std::ifstream input(RecentProjectsPath());
    std::string value;
    bool changed = false;
    while (std::getline(input, value))
    {
        if (projects.size() == 10)
        {
            changed = true;
            continue;
        }
        if (!IsValidProject(value))
        {
            changed = true;
            continue;
        }
        const std::string normalized = fs::absolute(value).lexically_normal().string();
        const bool duplicate = std::any_of(projects.begin(), projects.end(),
            [&normalized](const std::string& existing) {
#if defined(_WIN32)
                return _stricmp(existing.c_str(), normalized.c_str()) == 0;
#else
                return existing == normalized;
#endif
            });
        if (duplicate)
            changed = true;
        else
            projects.push_back(normalized);
    }
    if (changed)
        WriteRecentProjects(projects);
    return projects;
}

bool ProjectManager::CreateProjectDirectories(const std::string& projectPath)
{
    try
    {
        // 创建 Assets 目录
        fs::path assetsDir = fs::path(projectPath) / "Assets";
        fs::create_directories(assetsDir);

        // 创建 Scenes 目录
        fs::path scenesDir = fs::path(projectPath) / "Scenes";
        fs::create_directories(scenesDir);

        // 创建 Settings 目录
        fs::path settingsDir = fs::path(projectPath) / "Settings";
        fs::create_directories(settingsDir);

        // 创建 .gnx 隐藏目录（存放导入后的资源）
        fs::path gnxDir = fs::path(projectPath) / ".gnx";
        fs::create_directories(gnxDir);
        fs::create_directories(gnxDir / "Cache");

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
    std::string extension = fs::path(projectPath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    if (!fs::is_regular_file(projectPath) || extension != ".gnxproj")
        return false;
    ProjectConfig config;
    return config.LoadFromFile(projectPath);
}

NAMESPACE_GNXENGINE_END
