

set(mathutil_dir ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(mathutil_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB_RECURSE MATHUTIL_SOURCES CONFIGURE_DEPENDS ${mathutil_dir}/*.cpp ${mathutil_dir}/*.c ${mathutil_dir}/*.h)
file(GLOB_RECURSE MATHUTIL_INCLUDES CONFIGURE_DEPENDS ${mathutil_include_dir}/*.h)

# 设置源码文件列表
if(ANDROID)
    set(SOURCE_FILES ${MATHUTIL_SOURCES} ${MATHUTIL_INCLUDES} ${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c)
else()
    set(SOURCE_FILES ${MATHUTIL_SOURCES} ${MATHUTIL_INCLUDES})
endif()

add_library(MathUtil STATIC ${SOURCE_FILES})

source_group(TREE ${mathutil_dir} PREFIX source FILES ${MATHUTIL_SOURCES})
source_group(TREE ${mathutil_include_dir} PREFIX include FILES ${MATHUTIL_INCLUDES})

target_include_directories(MathUtil PRIVATE ${mathutil_include_dir})
if(ANDROID)
    target_include_directories(MathUtil PRIVATE ${ANDROID_NDK}/sources/android/cpufeatures)
endif()

target_compile_definitions(MathUtil PRIVATE MATH3D_EXPORTS)
target_link_libraries(MathUtil PRIVATE BaseLib)



