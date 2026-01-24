//
//  ProjectConfig.h
//  GNXEngine
//
//  工程配置管理
//

#ifndef GNXENGINE_PROJECT_CONFIG_INCLUDE_SDFJSDJH
#define GNXENGINE_PROJECT_CONFIG_INCLUDE_SDFJSDJH

#include "PreDefine.h"
#include <string>
#include <vector>

NAMESPACE_GNXENGINE_BEGIN

// 渲染路径
enum class RenderPath
{
    Forward,    // 前向渲染
    Deferred    // 延迟渲染
};

// 工程配置
struct GNXENGINE_API ProjectConfig
{
    // 基本信息
    std::string projectName;              // 项目名称
    std::string version = "1.0.0";       // 项目版本
    std::string creationDate;             // 创建日期
    std::string lastModified;            // 最后修改日期

    // 工程路径
    std::string projectPath;              // 工程根目录（包含 project.gnxproj 的路径）
    std::string assetsPath;               // Assets 目录
    std::string scenesPath;               // Scenes 目录
    std::string settingsPath;             // Settings 目录

    // 渲染设置
    RenderPath renderPath = RenderPath::Deferred;  // 默认延迟渲染
    uint32_t defaultWidth = 1280;        // 默认窗口宽度
    uint32_t defaultHeight = 720;        // 默认窗口高度
    bool vsyncEnabled = true;            // 垂直同步

    // 场景设置
    std::string defaultScene;            // 默认场景路径（相对于 Scenes 目录）
    std::vector<std::string> recentScenes; // 最近打开的场景列表

    // 序列化到 JSON
    std::string ToJson() const;

    // 从 JSON 加载
    bool FromJson(const std::string& json);

    // 保存到文件
    bool SaveToFile(const std::string& filepath);

    // 从文件加载
    bool LoadFromFile(const std::string& filepath);

    // 获取工程目录
    std::string GetProjectDirectory() const;

    // 获取绝对路径
    std::string GetAbsoluteAssetPath(const std::string& relativePath) const;
    std::string GetAbsoluteScenePath(const std::string& relativePath) const;
    std::string GetRelativeAssetPath(const std::string& absolutePath) const;
    std::string GetRelativeScenePath(const std::string& absolutePath) const;
};

// 工程管理器（单例）
class GNXENGINE_API ProjectManager
{
public:
    static ProjectManager& GetInstance();

    // 工程操作
    bool CreateNewProject(const std::string& projectPath, const std::string& projectName);
    bool OpenProject(const std::string& projectPath);
    bool CloseProject();
    bool SaveProject();
    bool IsProjectOpen() const { return mProject != nullptr; }

    // 获取当前工程配置
    ProjectConfig* GetProject() const { return mProject; }
    std::string GetProjectPath() const;

    // 最近工程列表
    void AddRecentProject(const std::string& projectPath);
    std::vector<std::string> GetRecentProjects() const;

    // 创建默认目录结构
    bool CreateProjectDirectories(const std::string& projectPath);

    // 验证工程文件
    bool IsValidProject(const std::string& projectPath) const;

private:
    ProjectManager();
    ~ProjectManager();

    ProjectConfig* mProject = nullptr;

    std::string mRecentProjectsFile = "GNXEngine_RecentProjects.ini";
};

NAMESPACE_GNXENGINE_END

#endif /* GNXENGINE_PROJECT_CONFIG_INCLUDE_SDFJSDJH */
