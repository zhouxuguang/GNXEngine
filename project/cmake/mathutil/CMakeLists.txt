

set(mathutil_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../source/MathUtil)
set(mathutil_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/MathUtil)
file(GLOB_RECURSE MATHUTIL_SOURCES CONFIGURE_DEPENDS ${mathutil_dir}/*.cpp ${mathutil_dir}/*.c ${mathutil_dir}/*.h)
file(GLOB_RECURSE MATHUTIL_INCLUDES CONFIGURE_DEPENDS ${mathutil_include_dir}/*.h)

add_library(mathutil OBJECT ${MATHUTIL_SOURCES} ${MATHUTIL_INCLUDES})
target_include_directories(mathutil PUBLIC ${mathutil_include_dir} ${CMAKE_CURRENT_SOURCE_DIR}/../../../include)
target_link_libraries(mathutil PRIVATE baselib)



