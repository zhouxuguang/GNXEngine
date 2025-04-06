

set(mathutil_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../source/MathUtil)
set(mathutil_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/MathUtil)
file(GLOB_RECURSE MATHUTIL_SOURCES CONFIGURE_DEPENDS ${mathutil_dir}/*.cpp ${mathutil_dir}/*.c ${mathutil_dir}/*.h)
file(GLOB_RECURSE MATHUTIL_INCLUDES CONFIGURE_DEPENDS ${mathutil_include_dir}/*.h)

# 设置源码文件列表
if(ANDROID)
    set(SOURCE_FILES ${MATHUTIL_SOURCES} ${MATHUTIL_INCLUDES} ${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c)
else()
    set(SOURCE_FILES ${MATHUTIL_SOURCES} ${MATHUTIL_INCLUDES})
endif()

add_library(mathutil STATIC ${SOURCE_FILES})

source_group(TREE ${mathutil_dir} FILES ${MATHUTIL_SOURCES})
source_group(TREE ${mathutil_include_dir} FILES ${MATHUTIL_INCLUDES})

target_include_directories(mathutil PUBLIC ${mathutil_include_dir} ${CMAKE_CURRENT_SOURCE_DIR}/../../../include)
if(ANDROID)
    target_include_directories(mathutil PUBLIC ${ANDROID_NDK}/sources/android/cpufeatures)
endif()

target_compile_definitions(mathutil PRIVATE MATH3D_EXPORTS)
target_link_libraries(mathutil PRIVATE baselib)



