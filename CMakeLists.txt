cmake_minimum_required(VERSION 3.17)

project(GNXEngine VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 修复MSVC的__cplusplus宏的值错误的问题
if(MSVC)
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(WIN32)
    # Windows: DLL 是运行时组件
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
elseif(APPLE)
    # macOS: dylib 是运行时组件
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
else()
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

# 设置默认的符号可见性
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

include(cmake/ISPC.cmake)

message("ISPC: ${CMAKE_ISPC_COMPILER}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Engine)

# 编译选项：单元测试和编辑器默认关闭，通过 -DENABLE_TESTING=ON / -DENABLE_EDITOR=ON 打开
option(ENABLE_TESTING "Build unit tests" OFF)
option(ENABLE_EDITOR "Build editor" OFF)

add_subdirectory(Engine/Runtime)

if(ENABLE_EDITOR)
    add_subdirectory(Engine/Editor)
endif()

add_subdirectory(ThirdParty)
add_subdirectory(test)
add_subdirectory(demo)

if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(unittest)
endif()

