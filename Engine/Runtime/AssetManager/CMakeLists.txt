
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

set_property(TARGET AssetManager PROPERTY FOLDER "Runtime")
