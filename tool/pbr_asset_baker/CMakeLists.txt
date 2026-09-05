add_executable(pbr_asset_baker
    main.cpp
)

# pbr_asset_baker is a host-only offline baking tool (macOS/Windows/Linux)
# It reuses AssetProcess (KTX generation + IBL baking), AssetManager (pb packing)
# and assimp (mesh parsing for meshasset). All engine modules are aggregated in
# the GNXEngine shared library (same pattern as test/ offline tools).
# ktx is required for direct KTX writing helpers; assimp for model parsing.
target_link_libraries(pbr_asset_baker PRIVATE GNXEngine ktx assimp)

# AssetProcess 内部源文件头（IBLBaker/PBRBase.h、TextureProcess/EnvHdrProcess.h
# 在 source/ 下）不作为 PUBLIC 接口暴露，这里显式添加。
# include 前缀如 "Runtime/AssetProcess/source/..." 从 Engine/ 解析，故加 Engine/ 根。
target_include_directories(pbr_asset_baker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Engine
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Engine/Runtime/AssetProcess/source
)

set_property(TARGET pbr_asset_baker PROPERTY FOLDER "Tool")
