cmake_minimum_required(VERSION 3.17)

project(GNXEngine VERSION 0.1 LANGUAGES C CXX)

add_subdirectory(baselib)
add_subdirectory(imagecodec)
add_subdirectory(mathutil)
add_subdirectory(rendercore)
add_subdirectory(rendersystem)
add_subdirectory(shadercompiler)
add_subdirectory(../../source/thirdparty/assimp assimp)
add_subdirectory(../../source/thirdparty/SPIRV-Reflect SPIRV-Reflect)



