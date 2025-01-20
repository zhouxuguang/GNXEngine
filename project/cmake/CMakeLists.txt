cmake_minimum_required(VERSION 3.17)

project(GNXEngine VERSION 0.1 LANGUAGES C CXX)

set(GNXEngine_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../source/GNXEngine)
set(GNXEngine_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../include/GNXEngine)
file(GLOB_RECURSE GNXEngine_Source CONFIGURE_DEPENDS ${GNXEngine_dir}/*.cpp ${GNXEngine_dir}/*.c ${GNXEngine_dir}/*.h)
file(GLOB_RECURSE GNXEngine_Include CONFIGURE_DEPENDS ${GNXEngine_include_dir}/*.h)

add_library(GNXEngine SHARED ${GNXEngine_Source} ${GNXEngine_Include})

message("GNXEngine_Include 的值是: ${GNXEngine_Include}")

target_include_directories(GNXEngine PUBLIC ${GNXEngine_include_dir})

target_link_libraries(GNXEngine PUBLIC baselib imagecodec mathutil rendercore rendersystem shadercompiler spirv_reflect tbb_static AssetProcess)

add_subdirectory(baselib)
add_subdirectory(imagecodec)
add_subdirectory(mathutil)
add_subdirectory(rendercore)
add_subdirectory(rendersystem)
add_subdirectory(shadercompiler)
add_subdirectory(assetprocess)
add_subdirectory(../../source/thirdparty/assimp assimp)
add_subdirectory(../../source/thirdparty/SPIRV-Reflect SPIRV-Reflect)
add_subdirectory(../../source/thirdparty/tbb tbb)
add_subdirectory(../../source/thirdparty/meshoptimizer meshoptimizer)
add_subdirectory(../../source/thirdparty/nanopb nanopb)
add_subdirectory(../../source/thirdparty/glfw glfw)
#add_subdirectory(../../source/thirdparty/tinyktx2 tinyktx2)
