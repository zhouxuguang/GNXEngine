cmake_minimum_required(VERSION 3.17)

# 尝试从系统中查找 Catch2，找不到则通过 FetchContent 下载
find_package(Catch2 QUIET)

if(NOT Catch2_FOUND)
    include(FetchContent)

    FetchContent_Declare(
      Catch2
      GIT_REPOSITORY https://github.com/catchorg/Catch2.git
      GIT_TAG        v3.3.2
    )
    FetchContent_MakeAvailable(Catch2)

    # 将 Catch2 相关目标放入 ThirdParty/Catch2 文件夹
    set_property(TARGET Catch2 PROPERTY FOLDER "ThirdParty/Catch2")
    set_property(TARGET Catch2WithMain PROPERTY FOLDER "ThirdParty/Catch2")
endif()

enable_testing()

# BaseLib 单元测试
add_executable(test_baselib test_baselib.cpp)
target_link_libraries(test_baselib PRIVATE Catch2::Catch2WithMain BaseLib)
target_compile_definitions(test_baselib PRIVATE MATH3D_EXPORTS)
set_property(TARGET test_baselib PROPERTY FOLDER "unittest")
source_group("Source Files" FILES test_baselib.cpp)

add_test(NAME test_baselib COMMAND test_baselib)

# MathUtil 单元测试
# MathUtil 的 PRIVATE 依赖 BaseLib 不会传递，需要显式链接
add_executable(test_mathutil test_mathutil.cpp)
target_link_libraries(test_mathutil PRIVATE Catch2::Catch2WithMain MathUtil BaseLib)
target_compile_definitions(test_mathutil PRIVATE MATH3D_EXPORTS)
set_property(TARGET test_mathutil PROPERTY FOLDER "unittest")
source_group("Source Files" FILES test_mathutil.cpp)

add_test(NAME test_mathutil COMMAND test_mathutil)
