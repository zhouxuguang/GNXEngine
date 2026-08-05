
set(asset_process_dir ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(asset_process_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB_RECURSE ASSET_PROCESS_SOURCES CONFIGURE_DEPENDS ${asset_process_dir}/*.cpp ${asset_process_dir}/*.c ${asset_process_dir}/*.h)
file(GLOB_RECURSE ASSET_PROCESS_INCLUDES CONFIGURE_DEPENDS ${asset_process_include_dir}/*.h)

# 设置 ISPC 目标架构
if(CMAKE_OSX_ARCHITECTURES MATCHES "arm64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|ARM64|aarch64|armv7|armv8)")
    set(ISPC_TARGET "neon-i32x4")
elseif(CMAKE_OSX_ARCHITECTURES MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)")
    set(ISPC_TARGET "sse2-i32x8,sse4.1-i32x8,avx1-i32x8,avx2-i32x8")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i.86|x86)")
    set(ISPC_TARGET "sse2-i32x8,sse4.1-i32x8")
else()
    message(WARNING "ISPC: Unknown processor '${CMAKE_SYSTEM_PROCESSOR}', using neon-i32x4 as fallback")
    set(ISPC_TARGET "neon-i32x4")
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

target_link_libraries(AssetProcess PRIVATE assimp RenderSystem nanopb tbb ktx pvrtc ImageCodec AssetManager yaml-cpp)

set_property(TARGET AssetProcess PROPERTY FOLDER "Runtime")
