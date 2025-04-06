
set(asset_process_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../source/AssetProcess)
set(asset_process_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/AssetProcess)
file(GLOB_RECURSE MESH_PROCESS_SOURCES CONFIGURE_DEPENDS ${asset_process_dir}/*.cpp ${asset_process_dir}/*.c ${asset_process_dir}/*.h)
file(GLOB_RECURSE MESH_PROCESS_INCLUDES CONFIGURE_DEPENDS ${asset_process_include_dir}/*.h)

include(../Common.cmake)

set(SOURCE_FILES ${MESH_PROCESS_SOURCES} ${MESH_PROCESS_INCLUDES})

add_library(AssetProcess STATIC ${SOURCE_FILES})

SETUP_GROUPS("${SOURCE_FILES}")

target_include_directories(AssetProcess PUBLIC ${asset_process_include_dir} ${CMAKE_CURRENT_SOURCE_DIR}/../../../include)

target_link_libraries(AssetProcess PUBLIC assimp rendersystem nanopb)

