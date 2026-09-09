#pragma once
#include <memory>
class AssetImportService;
class EditorProjectService;
class EditorRenderHost;
class EditorSettings;
struct EditorContext {
  std::unique_ptr<EditorProjectService> projectService;
  std::unique_ptr<AssetImportService> assetImportService;
  std::unique_ptr<EditorRenderHost> renderHost;
  std::unique_ptr<EditorSettings> settings;
};
