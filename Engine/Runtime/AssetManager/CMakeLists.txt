
set(assetmanager_dir ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(assetmanager_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 收集所有源文件
file(GLOB_RECURSE ASSETMANAGER_SOURCES CONFIGURE_DEPENDS
    ${assetmanager_dir}/*.cpp
    ${assetmanager_dir}/*.c)

file(GLOB_RECURSE ASSETMANAGER_INCLUDES CONFIGURE_DEPENDS
    ${assetmanager_include_dir}/*.h)

set(SOURCE_FILES ${ASSETMANAGER_SOURCES} ${ASSETMANAGER_INCLUDES})

add_library(AssetManager OBJECT ${SOURCE_FILES})

source_group(TREE ${assetmanager_dir} PREFIX source FILES ${ASSETMANAGER_SOURCES})
source_group(TREE ${assetmanager_include_dir} PREFIX include FILES ${ASSETMANAGER_INCLUDES})

target_include_directories(AssetManager
    PUBLIC
    ${assetmanager_include_dir}
    PRIVATE
    ${assetmanager_dir}     # 内部 pb 头 / PBUtils 等，不暴露给依赖方
)

# 链接依赖
target_link_libraries(AssetManager
    PUBLIC
    RenderCore
    nanopb
    PRIVATE
    BaseLib
    ImageCodec
)

# 移动端：AssetManager.cpp 在 GNX_OS_IOS || GNX_OS_ANDROID 下直接
# #include "SDL_rwops.h"（包内资源读取 SDL_RWFromFile），需要 SDL2 头文件。
# SDL2-static 仅在 ANDROID/IOS 下由 ThirdParty 编译，故加平台判断。
# 头文件只在 .cpp 内部使用，不外泄到 AssetManager 公共头，故用 PRIVATE。
if(ANDROID OR IOS)
    target_link_libraries(AssetManager PRIVATE SDL2-static)
endif()

set_property(TARGET AssetManager PROPERTY FOLDER "Runtime")
