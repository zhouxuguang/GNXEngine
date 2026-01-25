
set(asset_process_dir ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(asset_process_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB_RECURSE ASSET_PROCESS_SOURCES CONFIGURE_DEPENDS ${asset_process_dir}/*.cpp ${asset_process_dir}/*.c ${asset_process_dir}/*.h)
file(GLOB_RECURSE ASSET_PROCESS_INCLUDES CONFIGURE_DEPENDS ${asset_process_include_dir}/*.h)

# 设置 ISPC 目标架构
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(ISPC_TARGET "sse2-i32x4,sse4-i32x4,avx1-i32x8,avx2-i32x8,avx512knl-i32x16,avx512skx-i32x16")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|ARM64")
    set(ISPC_TARGET "neon")
endif()

# 添加 ISPC 源文件
set(ISPC_SOURCES
    ${asset_process_dir}/TextureProcess/kernel_astc.ispc
    ${asset_process_dir}/TextureProcess/kernel.ispc
)

# 编译 ISPC 文件
add_ispc_target(
    ALL_ISPC_BUILD_OUTPUT ISPC_HEADER_DIR
    SOURCES ${ISPC_SOURCES}
    TARGET ${ISPC_TARGET}
)

# 创建独立的 ISPC 压缩库
add_library(ISPCCompress STATIC)
target_sources(ISPCCompress PRIVATE
    ${asset_process_dir}/TextureProcess/ispc_texcomp.cpp
    ${asset_process_dir}/TextureProcess/ispc_texcomp_astc.cpp
    ${ALL_ISPC_BUILD_OUTPUT}
)

target_include_directories(ISPCCompress PRIVATE
    ${asset_process_include_dir}
    ${ISPC_HEADER_DIR}
    ${asset_process_dir}/TextureProcess
)

set_property(TARGET ISPCCompress PROPERTY FOLDER "Runtime")

# AssetProcess 库 - 排除 ISPC 相关的文件
file(GLOB_RECURSE ASSET_PROCESS_SOURCES CONFIGURE_DEPENDS ${asset_process_dir}/*.cpp ${asset_process_dir}/*.c ${asset_process_dir}/*.h)
file(GLOB_RECURSE ASSET_PROCESS_INCLUDES CONFIGURE_DEPENDS ${asset_process_include_dir}/*.h)

# 排除 ISPC 相关的 cpp 文件和 ispc 源文件
list(FILTER ASSET_PROCESS_SOURCES EXCLUDE REGEX ".*ispc_texcomp.*\\.cpp$")
list(FILTER ASSET_PROCESS_SOURCES EXCLUDE REGEX ".*ispc.*\\.ispc$")

set(SOURCE_FILES ${ASSET_PROCESS_SOURCES} ${ASSET_PROCESS_INCLUDES})

add_library(AssetProcess OBJECT ${SOURCE_FILES})

source_group(TREE ${asset_process_dir} PREFIX source FILES ${ASSET_PROCESS_SOURCES})
source_group(TREE ${asset_process_include_dir} PREFIX include FILES ${ASSET_PROCESS_INCLUDES})

target_include_directories(AssetProcess PRIVATE ${asset_process_include_dir} ${ISPC_HEADER_DIR})

target_link_libraries(AssetProcess PRIVATE assimp RenderSystem nanopb tbb ktx pvrtc ImageCodec AssetManager)

set_property(TARGET AssetProcess PROPERTY FOLDER "Runtime")
