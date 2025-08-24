
set(asset_process_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../source/AssetProcess)
set(asset_process_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/AssetProcess)
file(GLOB_RECURSE MESH_PROCESS_SOURCES CONFIGURE_DEPENDS ${asset_process_dir}/*.cpp ${asset_process_dir}/*.c ${asset_process_dir}/*.h)
file(GLOB_RECURSE MESH_PROCESS_INCLUDES CONFIGURE_DEPENDS ${asset_process_include_dir}/*.h)

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
    ALL_ISPC_BUILD_OUTPUT
    SOURCES ${ISPC_SOURCES}
    TARGET ${ISPC_TARGET}
)

set(SOURCE_FILES ${MESH_PROCESS_SOURCES} ${MESH_PROCESS_INCLUDES})

add_library(AssetProcess STATIC ${SOURCE_FILES} ${ALL_ISPC_BUILD_OUTPUT} ${ISPC_SOURCES})

source_group(TREE ${asset_process_dir} FILES ${MESH_PROCESS_SOURCES})
source_group(TREE ${asset_process_include_dir} FILES ${MESH_PROCESS_INCLUDES})

target_include_directories(AssetProcess PUBLIC ${asset_process_include_dir} ${CMAKE_CURRENT_SOURCE_DIR}/../../../include)

target_link_libraries(AssetProcess PUBLIC assimp rendersystem nanopb tbb ktx pvrtc imagecodec)

