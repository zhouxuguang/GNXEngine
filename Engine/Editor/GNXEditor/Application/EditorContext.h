#pragma once
#include <memory>
class AssetImportService;
class AssetRegistry;
class EditorProjectService;
class EditorRenderHost;
class EditorSettings;
class ThumbnailService;
class CommandHistory;
class SceneDocument;
class SelectionService;
struct EditorContext
{
    std::unique_ptr<EditorProjectService> projectService;
    std::unique_ptr<AssetImportService> assetImportService;
    std::unique_ptr<AssetRegistry> assetRegistry;
    std::unique_ptr<EditorRenderHost> renderHost;
    std::unique_ptr<EditorSettings> settings;
    std::unique_ptr<ThumbnailService> thumbnailService;
    std::unique_ptr<CommandHistory> commandHistory;
    std::unique_ptr<SceneDocument> sceneDocument;
    std::unique_ptr<SelectionService> selectionService;
};
