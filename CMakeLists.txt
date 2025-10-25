cmake_minimum_required(VERSION 3.17)

project(GNXEngine VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(cmake/ISPC.cmake)

message("ISPC: ${CMAKE_ISPC_COMPILER}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Engine)

add_subdirectory(Engine/Runtime)
add_subdirectory(Engine/Editor)
add_subdirectory(ThirdParty)
add_subdirectory(test) 

