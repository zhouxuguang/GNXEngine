# terrain demo 的 Android 库
# 编译为 SHARED 库（SDLActivity 通过 SDL 的 main 机制加载它）

# 从 src/ 往上 6 层到仓库根：src→jni→app→android→terrain→demo→根
set(GNX_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../)

add_library(terrain SHARED
    main.cpp
    ${GNX_ROOT}/demo/terrain/TerrainFrameWork.cpp
    # BRDF LUT 生成（纯 CPU，不依赖 ISPC；AssetProcess 在 Android 上被跳过，
    # 但 RenderSystem::ImageTextureUtil 引用了 AssetProcess::GenerateBRDFLUT，
    # 因此必须直接编译 PBRBase.cpp 提供该符号）
    ${GNX_ROOT}/Engine/Runtime/AssetProcess/source/IBLBaker/PBRBase.cpp
)

# 链接引擎核心库
target_include_directories(terrain PRIVATE
    ${GNX_ROOT}/Engine
    ${GNX_ROOT}/Engine/Runtime/AssetProcess/include   # PBRBase.h
    ${GNX_ROOT}/Engine/Runtime/AssetProcess/source    # TextureImporter.h
    ${GNX_ROOT}/Engine/Runtime/AssetProcess/source/IBLBaker
    ${GNX_ROOT}/ThirdParty/SDL2/SDL/include
    ${GNX_ROOT}/demo/terrain    # TerrainFrameWork.h
)

target_link_libraries(terrain
    GNXEngine
    SDL2-static
    android
    log
)

# Android 上必须保证产物名为 libterrain.so（不带 Debug 后缀 d）
# SDLActivity 通过 getLibraries() 里的 "terrain" 精确加载 libterrain.so
set_target_properties(terrain PROPERTIES
    DEBUG_POSTFIX ""
    OUTPUT_NAME "terrain"
)
