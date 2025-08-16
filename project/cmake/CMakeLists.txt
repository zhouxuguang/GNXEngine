cmake_minimum_required(VERSION 3.20)

project(GNXEngine VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    # 添加标志以启用__cplusplus宏的更新
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>")
endif()

include(cmake/ISPC.cmake)

message("ISPC: ${CMAKE_ISPC_COMPILER}")

set(GNXEngine_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../source/GNXEngine)
set(GNXEngine_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../include/GNXEngine)
file(GLOB_RECURSE GNXEngine_Source CONFIGURE_DEPENDS ${GNXEngine_dir}/*.cpp ${GNXEngine_dir}/*.c ${GNXEngine_dir}/*.h)
file(GLOB_RECURSE GNXEngine_Include CONFIGURE_DEPENDS ${GNXEngine_include_dir}/*.h)

add_library(GNXEngine SHARED ${GNXEngine_Source} ${GNXEngine_Include})

target_include_directories(GNXEngine PUBLIC ${GNXEngine_include_dir})

target_link_libraries(GNXEngine PUBLIC baselib imagecodec mathutil rendercore rendersystem shadercompiler spirv_reflect tbb AssetProcess)
target_compile_definitions(GNXEngine PRIVATE GNXENGINE_EXPORTS)

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
add_subdirectory(../../source/thirdparty/ktx ktx)
add_subdirectory(../../source/thirdparty/zlib zlib)
add_subdirectory(../../source/thirdparty/pvrtc pvrtc)
