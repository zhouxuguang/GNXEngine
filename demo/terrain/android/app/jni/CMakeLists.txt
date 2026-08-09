cmake_minimum_required(VERSION 3.6)

project(terrain_android)

# ============================================================
# GNXEngine Android 构建
# 复用引擎的 CMake 构建体系，但只编译核心库（Engine/Runtime + ThirdParty），
# 跳过 test / demo / tool 等桌面目标。
# ============================================================

# 引擎根目录（从 jni/ 往上 5 层到仓库根：jni→app→android→terrain→demo→根）
set(GNX_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../..)

# 与引擎根 CMakeLists.txt 保持一致的全局配置
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Android 强制所有代码编译为位置无关代码 (PIC)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 符号可见性（与桌面一致）
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# 编译选项（与引擎根一致）
option(ENABLE_TESTING "Build unit tests" OFF)
option(ENABLE_EDITOR "Build editor" OFF)
option(ENABLE_NSIGHT_AFTERMATH "Enable NVIDIA Nsight Aftermath GPU crash dump integration" OFF)
option(ENABLE_TRACY "Enable Tracy profiler instrumentation" OFF)

include_directories(${GNX_ROOT}/Engine)

# ISPC.cmake：以项目根目录（GNX_ROOT）定位 ISPC 编译器，
# 使 add_ispc_target 在子构建中也可用（AssetProcess 在 Android 上虽被跳过，但保持一致性）
include(${GNX_ROOT}/cmake/ISPC.cmake)

# 核心库：第三方库 + 引擎运行时
# 顺序很重要：先 ThirdParty（定义 SDL2 等），再 Engine/Runtime（链接它们）
add_subdirectory(${GNX_ROOT}/ThirdParty ${CMAKE_CURRENT_BINARY_DIR}/ThirdParty)
add_subdirectory(${GNX_ROOT}/Engine/Runtime ${CMAKE_CURRENT_BINARY_DIR}/Engine/Runtime)

# terrain demo 的 Android 可加载库（SDL_main 入口）
add_subdirectory(src)
