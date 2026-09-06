add_executable(pbr_asset_baker
    main.cpp
)

# pbr_asset_baker is a host-only offline baking tool (macOS/Windows/Linux).
# All packaging logic lives in the engine's AssetProcess::AssetPackager
# (model assimp parse, texture RGB->RGBA + BC7/BC6H, IBL bake, primitive meshes)
# and AssetManager::AssetContainerWriter (AssetFileHeader + pb container write).
# The tool links only GNXEngine and only does CLI parsing.
target_link_libraries(pbr_asset_baker PRIVATE GNXEngine)

# AssetPackager.h 是 AssetProcess 的 PUBLIC include，从 Engine/ 根解析。
target_include_directories(pbr_asset_baker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Engine
)

set_property(TARGET pbr_asset_baker PROPERTY FOLDER "Tool")
